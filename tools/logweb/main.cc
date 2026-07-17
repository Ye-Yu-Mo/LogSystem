// tools/logweb/main.cc —— Web 日志面板
//
// 最小 HTTP 服务端 + SSE 实时推送。单线程，零外部 HTTP 库依赖。
// 前端 HTML/CSS/JS 内嵌为 C++ 原始字符串字面量。

#include "../LogQuery.hpp"
#include <sqlite3.h>
#include <jsoncpp/json/json.h>

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <csignal>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// ── 最小 HTTP 工具 ────────────────────────────────────────────

static std::string urlDecode(const std::string &s)
{
    std::string r;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '%' && i + 2 < s.size())
        {
            int h;
            sscanf(s.c_str() + i + 1, "%2x", &h);
            r += (char)h;
            i += 2;
        }
        else if (s[i] == '+')
            r += ' ';
        else
            r += s[i];
    }
    return r;
}

static std::map<std::string, std::string> parseQuery(const std::string &qs)
{
    std::map<std::string, std::string> m;
    std::istringstream iss(qs);
    std::string pair;
    while (std::getline(iss, pair, '&'))
    {
        size_t eq = pair.find('=');
        if (eq != std::string::npos)
            m[pair.substr(0, eq)] = urlDecode(pair.substr(eq + 1));
        else
            m[pair] = "";
    }
    return m;
}

struct HttpRequest
{
    std::string method;
    std::string path;
    std::map<std::string, std::string> query;
};

static HttpRequest parseRequest(const std::string &raw)
{
    HttpRequest req;
    size_t end = raw.find("\r\n");
    if (end == std::string::npos)
        return req;
    std::string line = raw.substr(0, end);
    std::istringstream iss(line);
    std::string fullpath;
    iss >> req.method >> fullpath;

    size_t q = fullpath.find('?');
    if (q != std::string::npos)
    {
        req.path = fullpath.substr(0, q);
        req.query = parseQuery(fullpath.substr(q + 1));
    }
    else
    {
        req.path = fullpath;
    }
    return req;
}

static void sendResponse(int fd, int code, const std::string &contentType, const std::string &body,
                          const std::map<std::string, std::string> &extra = {})
{
    std::ostringstream hdr;
    hdr << "HTTP/1.1 " << code << " OK\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Access-Control-Allow-Origin: *\r\n";
    for (auto &[k, v] : extra)
        hdr << k << ": " << v << "\r\n";
    hdr << "Connection: close\r\n\r\n"
        << body;
    std::string s = hdr.str();
    ::send(fd, s.c_str(), s.size(), 0);
}

static void send404(int fd)
{
    sendResponse(fd, 404, "text/plain", "Not Found");
}

// ── API 处理 ──────────────────────────────────────────────────

static std::string apiLogs(sqlite3 *db, const std::map<std::string, std::string> &q)
{
    LogQuery::Params p;
    auto it = q.find("level");
    if (it != q.end()) p.level = it->second;
    it = q.find("keyword");
    if (it != q.end()) p.keyword = it->second;
    it = q.find("logger");
    if (it != q.end()) p.logger = it->second;
    it = q.find("tid");
    if (it != q.end()) p.tid = it->second;
    it = q.find("from_time");
    if (it != q.end()) p.from_time = it->second;
    it = q.find("to_time");
    if (it != q.end()) p.to_time = it->second;
    it = q.find("limit");
    if (it != q.end()) p.limit = std::stoi(it->second);
    it = q.find("offset");
    if (it != q.end()) p.offset = std::stoi(it->second);

    auto rows = LogQuery::queryLogs(db, p);
    Json::StreamWriterBuilder w;
    w["indentation"] = "";
    return Json::writeString(w, rows);
}

static std::string apiStats(sqlite3 *db, const std::map<std::string, std::string> &q)
{
    LogQuery::Params p;
    auto it = q.find("group_by");
    if (it != q.end()) p.group_by = it->second;
    it = q.find("from_time");
    if (it != q.end()) p.from_time = it->second;
    it = q.find("to_time");
    if (it != q.end()) p.to_time = it->second;

    auto stats = LogQuery::logStats(db, p);
    Json::StreamWriterBuilder w;
    w["indentation"] = "";
    return Json::writeString(w, stats);
}

static std::string apiLoggers(sqlite3 *db)
{
    auto list = LogQuery::listLoggers(db);
    Json::StreamWriterBuilder w;
    w["indentation"] = "";
    return Json::writeString(w, list);
}

// ── SSE（Server-Sent Events） ─────────────────────────────────

static void handleSSE(int fd, sqlite3 *db)
{
    std::string hdr = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/event-stream\r\n"
                      "Cache-Control: no-cache\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Connection: keep-alive\r\n\r\n";
    ::send(fd, hdr.c_str(), hdr.size(), 0);

    int lastId = 0;
    // 获取当前最大 id
    {
        sqlite3_stmt *s;
        sqlite3_prepare_v2(db, "SELECT COALESCE(MAX(id),0) FROM logs", -1, &s, nullptr);
        if (sqlite3_step(s) == SQLITE_ROW)
            lastId = sqlite3_column_int(s, 0);
        sqlite3_finalize(s);
    }

    // 每隔 2 秒检查新记录
    for (int i = 0; i < 150; i++) // 5 分钟超时
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        sqlite3_stmt *s;
        sqlite3_prepare_v2(db, "SELECT id, log_time, log_level, logger_name, message FROM logs WHERE id > ? ORDER BY id ASC LIMIT 50",
                           -1, &s, nullptr);
        sqlite3_bind_int(s, 1, lastId);

        std::ostringstream data;
        bool has = false;
        while (sqlite3_step(s) == SQLITE_ROW)
        {
            if (!has) { data << "data: ["; has = true; }
            else data << ",";

            int id = sqlite3_column_int(s, 0);
            data << "{\"id\":" << id
                 << ",\"time\":\"" << sqlite3_column_text(s, 1) << "\""
                 << ",\"level\":\"" << sqlite3_column_text(s, 2) << "\""
                 << ",\"logger\":\"" << sqlite3_column_text(s, 3) << "\""
                 << ",\"msg\":\"" << sqlite3_column_text(s, 4) << "\"}";
            lastId = std::max(lastId, id);
        }
        sqlite3_finalize(s);

        if (has)
        {
            data << "]\n\n";
            std::string d = data.str();
            if (::send(fd, d.c_str(), d.size(), 0) < 0) break;
        }
        else
        {
            // 发送心跳保持连接
            std::string ping = ": heartbeat\n\n";
            if (::send(fd, ping.c_str(), ping.size(), 0) < 0) break;
        }
    }
}

// ── HTML 前端（内嵌） ─────────────────────────────────────────

static const char *HTML = R"html(<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LogSystem Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font:14px/1.5 system-ui,sans-serif;background:#1a1a2e;color:#e0e0e0;display:flex;flex-direction:column;height:100vh}
header{background:#16213e;padding:12px 20px;display:flex;gap:20px;align-items:center;border-bottom:1px solid #0f3460}
header h1{font-size:18px;color:#e94560;margin-right:auto}
.stat{padding:6px 14px;background:#0f3460;border-radius:6px;font-size:13px}
.stat b{color:#e94560}
main{display:flex;flex:1;overflow:hidden}
#stream{flex:1;overflow-y:auto;padding:10px}
#search{width:320px;background:#16213e;padding:16px;display:flex;flex-direction:column;gap:12px;border-left:1px solid #0f3460}
#search h3{color:#e94560;margin-bottom:4px}
#search input,#search select,#search button{padding:8px;border:1px solid #0f3460;border-radius:4px;background:#1a1a2e;color:#e0e0e0;font-size:13px}
#search button{background:#e94560;color:#fff;border:none;cursor:pointer;font-weight:bold}
#search button:hover{background:#c73652}
#results{flex:1;overflow-y:auto;margin-top:8px}
.log{display:flex;gap:8px;padding:3px 8px;border-bottom:1px solid #16213e;font-size:12px;animation:fadeIn .3s}
.log .time{color:#888;white-space:nowrap}
.log .level{font-weight:bold;min-width:44px}
.log .msg{flex:1;word-break:break-all}
.lv-DEBUG{color:#888}
.lv-INFO{color:#4ecca3}
.lv-WARN{color:#f0a500}
.lv-ERROR{color:#e94560}
.lv-FATAL{color:#c350cd}
@keyframes fadeIn{from{background:#e9456033}to{background:0}}
#results .log{border-left:2px solid transparent}
#results .lv-ERROR{border-left-color:#e94560}
#results .lv-FATAL{border-left-color:#c350cd}
</style>
</head>
<body>
<header>
<h1>📊 LogSystem</h1>
<span class="stat">Total <b id="st-total">-</b></span>
<span class="stat">ERROR <b id="st-error">-</b></span>
<span class="stat">WARN <b id="st-warn">-</b></span>
</header>
<main>
<div id="stream"></div>
<div id="search">
<h3>🔍 历史查询</h3>
<select id="q-level"><option value="">全部等级</option><option>DEBUG</option><option>INFO</option><option>WARN</option><option>ERROR</option><option>FATAL</option></select>
<input id="q-keyword" placeholder="关键词搜索...">
<input id="q-from" placeholder="起始时间 YYYY-MM-DD HH:MM">
<input id="q-to" placeholder="结束时间 YYYY-MM-DD HH:MM">
<button onclick="doSearch()">搜索</button>
<div id="results"></div>
</div>
</main>
<script>
const R = (id) => document.getElementById(id);
function fmtTime(t) { return (t||'').substring(0,19); }
function addLog(el, el2, d, animate) {
  let cls = 'lv-' + (d.level || d.log_level || '');
  let h = '<div class="log' + (animate?'':'') + '"><span class="time">' + fmtTime(d.time||d.log_time)
    + '</span><span class="level ' + cls + '">' + (d.level||d.log_level)
    + '</span><span class="msg">' + (d.msg||d.message||'') + '</span></div>';
  el.insertAdjacentHTML('afterbegin', h);
  if (el2) el2.insertAdjacentHTML('afterbegin', h);
  if (el.children.length > 200) { el.removeChild(el.lastChild); if(el2) el2.removeChild(el2.lastChild); }
}
function loadStats() {
  fetch('/api/stats?group_by=level').then(r=>r.json()).then(d=>{
    R('st-total').textContent = d.total || 0;
    let groups = d.groups || [];
    R('st-error').textContent = (groups.find(g=>g.group=='ERROR')||{}).count||0;
    R('st-warn').textContent = (groups.find(g=>g.group=='WARN')||{}).count||0;
  });
}
function doSearch() {
  let p = new URLSearchParams();
  let v = R('q-level').value; if(v) p.set('level',v);
  v = R('q-keyword').value; if(v) p.set('keyword',v);
  v = R('q-from').value; if(v) p.set('from_time',v);
  v = R('q-to').value; if(v) p.set('to_time',v);
  p.set('limit','100');
  fetch('/api/logs?'+p).then(r=>r.json()).then(rows=>{
    R('results').innerHTML = '';
    rows.forEach(r=>addLog(R('results'),null,r,false));
  });
}
// SSE 实时推送
let es = new EventSource('/api/stream');
es.onmessage = function(e) {
  try { JSON.parse(e.data).forEach(d=>addLog(R('stream'),null,d,true)); loadStats(); } catch(_) {}
};
loadStats();
</script>
</body>
</html>)html";

// ── 主循环 ────────────────────────────────────────────────────

static std::atomic<bool> g_running{true};

static void signalHandler(int) { g_running = false; }

int main(int argc, char *argv[])
{
    int port = 8080;
    std::string db_path = "data/log.db";

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc)
            port = std::stoi(argv[++i]);
        else if (arg == "--db" && i + 1 < argc)
            db_path = argv[++i];
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) { std::cerr << "socket failed\n"; return 1; }

    int opt = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (::bind(listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "bind port " << port << " failed\n";
        return 1;
    }
    if (::listen(listenFd, 10) < 0) { std::cerr << "listen failed\n"; return 1; }

    std::cout << "[logweb] http://localhost:" << port << "  db=" << db_path << std::endl;

    while (g_running)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(listenFd, &fds);
        struct timeval tv = {1, 0};
        int ret = select(listenFd + 1, &fds, nullptr, nullptr, &tv);
        if (ret < 0) break;
        if (ret == 0) continue;

        int clientFd = ::accept(listenFd, nullptr, nullptr);
        if (clientFd < 0) continue;

        // 读取请求（单线程，阻塞式处理）
        char buf[8192] = {};
        ssize_t n = ::recv(clientFd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { ::close(clientFd); continue; }
        buf[n] = '\0';

        auto req = parseRequest(std::string(buf));

        // 每个请求独立打开数据库（简单、线程安全）
        sqlite3 *db = nullptr;
        sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);

        if (req.path == "/" || req.path == "/index.html")
        {
            sendResponse(clientFd, 200, "text/html; charset=utf-8", HTML);
        }
        else if (req.path == "/api/logs")
        {
            std::string json = db ? apiLogs(db, req.query) : "[]";
            sendResponse(clientFd, 200, "application/json", json);
        }
        else if (req.path == "/api/stats")
        {
            std::string json = db ? apiStats(db, req.query) : "{}";
            sendResponse(clientFd, 200, "application/json", json);
        }
        else if (req.path == "/api/loggers")
        {
            std::string json = db ? apiLoggers(db) : "[]";
            sendResponse(clientFd, 200, "application/json", json);
        }
        else if (req.path == "/api/stream")
        {
            if (db) handleSSE(clientFd, db);
        }
        else
        {
            send404(clientFd);
        }

        if (db) sqlite3_close(db);
        ::close(clientFd);
    }

    ::close(listenFd);
    std::cout << "[logweb] stopped\n";
    return 0;
}
