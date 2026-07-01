// tools/logcli/main.cc —— 日志查询 CLI 工具
//
// 用法: ./logcli --db <path> [--level ERROR,WARN] [--from "2026-07-01 10:00"]
//                [--to "2026-07-01 12:00"] [--grep keyword] [--logger name]
//                [--tid 0x1234] [--limit 100] [--offset 0]
//                [--stats] [--group-by level|logger|hour]
//                [--format table|json|csv]
#include "../LogQuery.hpp"
#include <sqlite3.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <string>
#include <cstring>

static void usage(const char *prog)
{
    std::cerr << "Usage: " << prog << " --db <path> [options]\n"
              << "  --db PATH        数据库文件路径（必填）\n"
              << "  --level STR      日志等级过滤，逗号分隔（ERROR,WARN）\n"
              << "  --from TIME      起始时间 YYYY-MM-DD HH:MM\n"
              << "  --to TIME        结束时间 YYYY-MM-DD HH:MM\n"
              << "  --grep STR       关键词搜索\n"
              << "  --logger STR     日志器名称\n"
              << "  --tid STR        线程 ID\n"
              << "  --limit N        返回条数上限（默认1000）\n"
              << "  --offset N       偏移量（默认0）\n"
              << "  --stats          聚合统计模式\n"
              << "  --group-by STR   分组维度: level / logger / hour\n"
              << "  --format STR     输出格式: table / json / csv（默认table）\n";
}

// 表格输出
static void printTable(const Json::Value &rows)
{
    if (rows.empty())
    {
        std::cout << "(no results)\n";
        return;
    }
    // 简易对齐表格
    std::cout << "ID\tTIME\t\t\tLEVEL\tLOGGER\t\tMESSAGE\n";
    std::cout << "──\t────\t\t\t─────\t──────\t\t───────\n";
    for (auto &r : rows)
    {
        std::string time = r["log_time"].asString();
        if (time.size() > 19) time = time.substr(0, 19);
        std::string msg = r["message"].asString();
        if (msg.size() > 60) msg = msg.substr(0, 57) + "...";
        std::cout << r["id"].asInt() << "\t"
                  << time << "\t"
                  << r["log_level"].asString() << "\t"
                  << r["logger_name"].asString() << "\t\t"
                  << msg << "\n";
    }
    std::cout << "──\n" << rows.size() << " row(s)\n";
}

// 统计输出
static void printStats(const Json::Value &result, const std::string &format)
{
    if (format == "json")
    {
        Json::StreamWriterBuilder w;
        w["indentation"] = "";
        std::cout << Json::writeString(w, result) << "\n";
        return;
    }
    std::cout << "GROUP\t\tCOUNT\n─────\t\t─────\n";
    for (auto &g : result["groups"])
        std::cout << g["group"].asString() << "\t\t" << g["count"].asInt() << "\n";
    std::cout << "──\nTotal: " << result["total"].asInt() << "\n";
}

int main(int argc, char *argv[])
{
    std::string db_path, level, from_time, to_time, keyword, logger, tid, group_by, format = "table";
    int limit = 1000, offset = 0;
    bool stats = false;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--db" && i + 1 < argc) db_path = argv[++i];
        else if (arg == "--level" && i + 1 < argc) level = argv[++i];
        else if (arg == "--from" && i + 1 < argc) from_time = argv[++i];
        else if (arg == "--to" && i + 1 < argc) to_time = argv[++i];
        else if (arg == "--grep" && i + 1 < argc) keyword = argv[++i];
        else if (arg == "--logger" && i + 1 < argc) logger = argv[++i];
        else if (arg == "--tid" && i + 1 < argc) tid = argv[++i];
        else if (arg == "--limit" && i + 1 < argc) limit = std::stoi(argv[++i]);
        else if (arg == "--offset" && i + 1 < argc) offset = std::stoi(argv[++i]);
        else if (arg == "--group-by" && i + 1 < argc) group_by = argv[++i];
        else if (arg == "--format" && i + 1 < argc) format = argv[++i];
        else if (arg == "--stats") stats = true;
        else if (arg == "--help") { usage(argv[0]); return 0; }
        else { std::cerr << "未知参数: " << arg << "\n"; usage(argv[0]); return 1; }
    }

    if (db_path.empty()) { usage(argv[0]); return 1; }

    sqlite3 *db = nullptr;
    if (sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        std::cerr << "无法打开数据库: " << db_path << " (" << sqlite3_errmsg(db) << ")\n";
        return 1;
    }

    LogQuery::Params p;
    p.level = level;
    p.from_time = from_time;
    p.to_time = to_time;
    p.keyword = keyword;
    p.logger = logger;
    p.tid = tid;
    p.limit = limit;
    p.offset = offset;
    p.group_by = group_by;

    int rc = 0;
    if (stats)
    {
        auto result = LogQuery::logStats(db, p);
        if (format == "json")
            printStats(result, "json");
        else if (format == "csv")
        {
            std::cout << "group,count\n";
            for (auto &g : result["groups"])
                std::cout << g["group"].asString() << "," << g["count"].asInt() << "\n";
        }
        else
            printStats(result, "table");
    }
    else
    {
        auto rows = LogQuery::queryLogs(db, p);
        if (format == "json")
        {
            Json::StreamWriterBuilder w;
            w["indentation"] = "  ";
            std::cout << Json::writeString(w, rows) << "\n";
        }
        else if (format == "csv")
        {
            std::cout << "id,time,level,logger,file,line,tid,message\n";
            for (auto &r : rows)
                std::cout << r["id"].asInt() << ","
                          << r["log_time"].asString() << ","
                          << r["log_level"].asString() << ","
                          << r["logger_name"].asString() << ","
                          << r["source_file"].asString() << ","
                          << r["line_number"].asInt() << ","
                          << r["thread_id"].asString() << ","
                          << "\"" << r["message"].asString() << "\"\n";
        }
        else
            printTable(rows);
    }

    sqlite3_close(db);
    return rc;
}
