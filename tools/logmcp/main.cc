// tools/logmcp/main.cc —— MCP 日志查询服务端
//
// stdio JSON-RPC 2.0 传输，stdin 读请求，stdout 写响应，stderr 打自身日志。
// 暴露三个 Tool：query_logs / log_stats / list_loggers
#include "../LogQuery.hpp"
#include <sqlite3.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>

// ── JSON-RPC helpers ──────────────────────────────────────────

static Json::Value makeResponse(const Json::Value &id, const Json::Value &result)
{
    Json::Value r;
    r["jsonrpc"] = "2.0";
    r["id"] = id;
    r["result"] = result;
    return r;
}

static Json::Value makeError(const Json::Value &id, int code, const std::string &msg)
{
    Json::Value r;
    r["jsonrpc"] = "2.0";
    r["id"] = id;
    r["error"]["code"] = code;
    r["error"]["message"] = msg;
    return r;
}

static void sendJson(const Json::Value &v)
{
    Json::StreamWriterBuilder w;
    w["indentation"] = "";
    std::string s = Json::writeString(w, v);
    std::cout << s << "\n" << std::flush;
}

// ── Tool 定义 ─────────────────────────────────────────────────

static Json::Value toolList()
{
    Json::Value tools(Json::arrayValue);

    {
        Json::Value t;
        t["name"] = "query_logs";
        t["description"] = "Query log entries from the SQLite database. "
                           "Filter by log level, time range, keyword search, logger name, or thread ID. "
                           "Returns matching log entries with all fields.";
        t["inputSchema"]["type"] = "object";
        t["inputSchema"]["properties"]["db_path"]["type"] = "string";
        t["inputSchema"]["properties"]["db_path"]["description"] = "Path to the SQLite log database file";
        t["inputSchema"]["properties"]["level"]["type"] = "string";
        t["inputSchema"]["properties"]["level"]["description"] = "Comma-separated log levels (DEBUG,INFO,WARN,ERROR,FATAL)";
        t["inputSchema"]["properties"]["from_time"]["type"] = "string";
        t["inputSchema"]["properties"]["from_time"]["description"] = "Start time in format YYYY-MM-DD HH:MM";
        t["inputSchema"]["properties"]["to_time"]["type"] = "string";
        t["inputSchema"]["properties"]["to_time"]["description"] = "End time in format YYYY-MM-DD HH:MM";
        t["inputSchema"]["properties"]["keyword"]["type"] = "string";
        t["inputSchema"]["properties"]["keyword"]["description"] = "Keyword to search in log messages";
        t["inputSchema"]["properties"]["logger"]["type"] = "string";
        t["inputSchema"]["properties"]["logger"]["description"] = "Logger name filter";
        t["inputSchema"]["properties"]["tid"]["type"] = "string";
        t["inputSchema"]["properties"]["tid"]["description"] = "Thread ID filter";
        t["inputSchema"]["properties"]["limit"]["type"] = "integer";
        t["inputSchema"]["properties"]["limit"]["description"] = "Maximum number of results (default 1000)";
        t["inputSchema"]["properties"]["offset"]["type"] = "integer";
        t["inputSchema"]["properties"]["offset"]["description"] = "Result offset for pagination";
        t["inputSchema"]["required"] = Json::Value(Json::arrayValue);
        t["inputSchema"]["required"].append("db_path");
        tools.append(t);
    }
    {
        Json::Value t;
        t["name"] = "log_stats";
        t["description"] = "Get aggregated statistics from the log database. "
                           "Group counts by log level, logger name, or hour. "
                           "Returns group names with counts and total.";
        t["inputSchema"]["type"] = "object";
        t["inputSchema"]["properties"]["db_path"]["type"] = "string";
        t["inputSchema"]["properties"]["db_path"]["description"] = "Path to the SQLite log database file";
        t["inputSchema"]["properties"]["from_time"]["type"] = "string";
        t["inputSchema"]["properties"]["from_time"]["description"] = "Start time in format YYYY-MM-DD HH:MM";
        t["inputSchema"]["properties"]["to_time"]["type"] = "string";
        t["inputSchema"]["properties"]["to_time"]["description"] = "End time in format YYYY-MM-DD HH:MM";
        t["inputSchema"]["properties"]["group_by"]["type"] = "string";
        t["inputSchema"]["properties"]["group_by"]["description"] = "Grouping dimension: level, logger, or hour";
        t["inputSchema"]["required"] = Json::Value(Json::arrayValue);
        t["inputSchema"]["required"].append("db_path");
        tools.append(t);
    }
    {
        Json::Value t;
        t["name"] = "list_loggers";
        t["description"] = "List all logger names present in the log database.";
        t["inputSchema"]["type"] = "object";
        t["inputSchema"]["properties"]["db_path"]["type"] = "string";
        t["inputSchema"]["properties"]["db_path"]["description"] = "Path to the SQLite log database file";
        t["inputSchema"]["required"] = Json::Value(Json::arrayValue);
        t["inputSchema"]["required"].append("db_path");
        tools.append(t);
    }
    return tools;
}

// ── 请求分发 ──────────────────────────────────────────────────

static bool validatePath(const std::string &path)
{
    // 拒绝路径遍历，其他都放行
    if (path.find("..") != std::string::npos) return false;
    return !path.empty();
}

static Json::Value handleCall(const Json::Value &params, const std::string &db_path_hint = "")
{
    std::string name = params.get("name", "").asString();
    Json::Value args = params.get("arguments", Json::Value(Json::objectValue));
    std::string db_path = args.get("db_path", db_path_hint).asString();

    if (!validatePath(db_path))
    {
        Json::Value err;
        err["isError"] = true;
        err["content"] = Json::Value(Json::arrayValue);
        Json::Value c;
        c["type"] = "text";
        c["text"] = "Invalid database path";
        err["content"].append(c);
        return err;
    }

    sqlite3 *db = nullptr;
    if (sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        std::string errmsg = "Cannot open database: " + db_path;
        if (db) { errmsg += " (" + std::string(sqlite3_errmsg(db)) + ")"; sqlite3_close(db); }
        Json::Value err;
        err["isError"] = true;
        err["content"] = Json::Value(Json::arrayValue);
        Json::Value c;
        c["type"] = "text";
        c["text"] = errmsg;
        err["content"].append(c);
        return err;
    }

    LogQuery::Params p;
    p.level = args.get("level", "").asString();
    p.from_time = args.get("from_time", "").asString();
    p.to_time = args.get("to_time", "").asString();
    p.keyword = args.get("keyword", "").asString();
    p.logger = args.get("logger", "").asString();
    p.tid = args.get("tid", "").asString();
    p.limit = args.get("limit", 1000).asInt();
    p.offset = args.get("offset", 0).asInt();
    p.group_by = args.get("group_by", "").asString();

    Json::Value result;
    if (name == "query_logs")
    {
        auto rows = LogQuery::queryLogs(db, p);
        Json::StreamWriterBuilder w;
        w["indentation"] = "  ";
        std::string json = Json::writeString(w, rows);
        result["content"] = Json::Value(Json::arrayValue);
        Json::Value c;
        c["type"] = "text";
        c["text"] = json;
        result["content"].append(c);
    }
    else if (name == "log_stats")
    {
        auto stats = LogQuery::logStats(db, p);
        Json::StreamWriterBuilder w;
        w["indentation"] = "  ";
        std::string json = Json::writeString(w, stats);
        result["content"] = Json::Value(Json::arrayValue);
        Json::Value c;
        c["type"] = "text";
        c["text"] = json;
        result["content"].append(c);
    }
    else if (name == "list_loggers")
    {
        auto list = LogQuery::listLoggers(db);
        Json::StreamWriterBuilder w;
        w["indentation"] = "  ";
        std::string json = Json::writeString(w, list);
        result["content"] = Json::Value(Json::arrayValue);
        Json::Value c;
        c["type"] = "text";
        c["text"] = json;
        result["content"].append(c);
    }
    else
    {
        result["isError"] = true;
        result["content"] = Json::Value(Json::arrayValue);
        Json::Value c;
        c["type"] = "text";
        c["text"] = "Unknown tool: " + name;
        result["content"].append(c);
    }

    sqlite3_close(db);
    return result;
}

// ── main ──────────────────────────────────────────────────────

int main()
{
    // stderr 用于自身日志，stdout 只走 JSON-RPC
    std::cerr << "[logmcp] started, waiting for JSON-RPC requests on stdin" << std::endl;

    std::string db_path_hint; // 可选的默认 db 路径

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line.empty()) continue;

        Json::Value req;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::istringstream iss(line);
        if (!Json::parseFromStream(reader, iss, &req, &errs))
        {
            std::cerr << "[logmcp] parse error: " << errs << std::endl;
            continue;
        }

        std::string method = req.get("method", "").asString();
        Json::Value id = req.get("id", Json::Value());

        if (method == "initialize")
        {
            Json::Value caps;
            caps["protocolVersion"] = "2024-11-05";
            caps["capabilities"]["tools"] = Json::Value(Json::objectValue);
            caps["serverInfo"]["name"] = "logmcp";
            caps["serverInfo"]["version"] = "1.0.0";
            db_path_hint = req["params"].get("db_path", "").asString();
            sendJson(makeResponse(id, caps));
        }
        else if (method == "notifications/initialized")
        {
            // 无需响应
        }
        else if (method == "tools/list")
        {
            Json::Value r;
            r["tools"] = toolList();
            sendJson(makeResponse(id, r));
        }
        else if (method == "tools/call")
        {
            auto result = handleCall(req["params"], db_path_hint);
            if (result.isMember("isError"))
            {
                result.removeMember("isError");
                sendJson(makeResponse(id, result)); // isError=true 时 result 即 content
            }
            else
            {
                sendJson(makeResponse(id, result));
            }
        }
        else if (method == "ping")
        {
            sendJson(makeResponse(id, Json::Value(Json::objectValue)));
        }
        else
        {
            sendJson(makeError(id, -32601, "Method not found: " + method));
        }
    }

    std::cerr << "[logmcp] exiting" << std::endl;
    return 0;
}
