// tools/LogQuery.hpp —— 共享日志查询引擎
//
// CLI 和 MCP 服务端共用的查询逻辑。
// 所有用户可控值走 sqlite3 参数绑定，不拼字符串。
#pragma once

#include <sqlite3.h>
#include <jsoncpp/json/json.h>
#include <string>
#include <vector>
#include <sstream>
#include <set>

namespace LogQuery
{
    struct Params
    {
        std::string level;     // 逗号分隔多等级 "ERROR,WARN"
        std::string from_time; // "YYYY-MM-DD HH:MM"
        std::string to_time;   // "YYYY-MM-DD HH:MM"
        std::string keyword;   // 全文搜索（LIKE %keyword%）
        std::string logger;    // 日志器名称
        std::string tid;       // 线程 ID
        int limit = 1000;      // 默认上限，避免返回巨量数据
        int offset = 0;
        std::string group_by;  // level / logger / hour（仅 logStats 使用）
    };

    namespace
    {
        // 帮劣：参数绑定的 sqlite3_prepare_v2 包装
        class BoundStmt
        {
            sqlite3 *_db;
            sqlite3_stmt *_stmt = nullptr;
            int _idx = 1;

        public:
            BoundStmt(sqlite3 *db, const std::string &sql) : _db(db)
            {
                sqlite3_prepare_v2(_db, sql.c_str(), -1, &_stmt, nullptr);
            }
            ~BoundStmt() { if (_stmt) sqlite3_finalize(_stmt); }
            operator bool() const { return _stmt != nullptr; }
            sqlite3_stmt *get() { return _stmt; }

            void bindInt(int val) { sqlite3_bind_int(_stmt, _idx++, val); }
            void bindInt64(long long val) { sqlite3_bind_int64(_stmt, _idx++, val); }
            void bindText(const std::string &val)
            {
                sqlite3_bind_text(_stmt, _idx++, val.c_str(), -1, SQLITE_TRANSIENT);
            }
        };

        // 将 "YYYY-MM-DD HH:MM" 转为 SQLite 可比较的文本
        // 不足 17 位则补 ":00"（秒），满足 TEXT 排序
        std::string padTime(const std::string &s)
        {
            if (s.empty())
                return "";
            if (s.size() >= 19)
                return s;
            return s + ":00"; // HH:MM → HH:MM:00
        }
    } // anonymous namespace

    /// @brief 按条件检索日志
    inline Json::Value queryLogs(sqlite3 *db, const Params &p)
    {
        std::string sql = "SELECT id, log_time, line_number, thread_id, log_level, "
                          "source_file, logger_name, message FROM logs WHERE 1=1";
        std::vector<std::string> texts;
        std::vector<int> ints;

        // 等级过滤：level="ERROR,WARN" → WHERE log_level IN (?,?)
        if (!p.level.empty())
        {
            std::istringstream iss(p.level);
            std::string token;
            std::vector<std::string> levels;
            while (std::getline(iss, token, ','))
                levels.push_back(token);

            sql += " AND log_level IN (";
            for (size_t i = 0; i < levels.size(); i++)
            {
                if (i > 0)
                    sql += ",";
                sql += "?";
                texts.push_back(levels[i]);
            }
            sql += ")";
        }

        if (!p.keyword.empty())
        {
            sql += " AND message LIKE ?";
            texts.push_back("%" + p.keyword + "%");
        }
        if (!p.logger.empty())
        {
            sql += " AND logger_name = ?";
            texts.push_back(p.logger);
        }
        if (!p.tid.empty())
        {
            sql += " AND thread_id = ?";
            texts.push_back(p.tid);
        }
        if (!p.from_time.empty())
        {
            sql += " AND log_time >= ?";
            texts.push_back(padTime(p.from_time));
        }
        if (!p.to_time.empty())
        {
            sql += " AND log_time <= ?";
            texts.push_back(padTime(p.to_time));
        }

        sql += " ORDER BY id ASC";
        sql += " LIMIT ? OFFSET ?";

        BoundStmt stmt(db, sql);
        if (!stmt)
            return Json::Value(Json::arrayValue);

        for (auto &t : texts)
            stmt.bindText(t);
        for (auto &i : ints)
            stmt.bindInt(i);
        stmt.bindInt(p.limit);
        stmt.bindInt(p.offset);

        Json::Value rows(Json::arrayValue);
        while (sqlite3_step(stmt.get()) == SQLITE_ROW)
        {
            Json::Value row;
            row["id"] = sqlite3_column_int(stmt.get(), 0);
            row["log_time"] = (const char *)sqlite3_column_text(stmt.get(), 1);
            row["line_number"] = sqlite3_column_int(stmt.get(), 2);
            row["thread_id"] = (const char *)sqlite3_column_text(stmt.get(), 3);
            row["log_level"] = (const char *)sqlite3_column_text(stmt.get(), 4);
            row["source_file"] = (const char *)sqlite3_column_text(stmt.get(), 5);
            row["logger_name"] = (const char *)sqlite3_column_text(stmt.get(), 6);
            row["message"] = (const char *)sqlite3_column_text(stmt.get(), 7);
            rows.append(row);
        }
        return rows;
    }

    /// @brief 聚合统计
    inline Json::Value logStats(sqlite3 *db, const Params &p)
    {
        std::string group_col;
        if (p.group_by == "level")
            group_col = "log_level";
        else if (p.group_by == "logger")
            group_col = "logger_name";
        else if (p.group_by == "hour")
            group_col = "strftime('%Y-%m-%d %H:00', log_time)";
        else
            group_col = "log_level";

        std::string sql = "SELECT " + group_col + " AS grp, COUNT(*) AS cnt FROM logs WHERE 1=1";

        BoundStmt stmt(db, sql + " GROUP BY grp ORDER BY cnt DESC");
        if (!stmt)
            return Json::Value(Json::objectValue);

        Json::Value result(Json::objectValue);
        Json::Value groups(Json::arrayValue);
        int total = 0;

        while (sqlite3_step(stmt.get()) == SQLITE_ROW)
        {
            Json::Value g;
            g["group"] = (const char *)sqlite3_column_text(stmt.get(), 0);
            g["count"] = sqlite3_column_int(stmt.get(), 1);
            groups.append(g);
            total += sqlite3_column_int(stmt.get(), 1);
        }
        result["groups"] = groups;
        result["total"] = total;
        return result;
    }

    /// @brief 列出所有日志器名称
    inline Json::Value listLoggers(sqlite3 *db)
    {
        BoundStmt stmt(db, "SELECT DISTINCT logger_name FROM logs ORDER BY logger_name");
        if (!stmt)
            return Json::Value(Json::arrayValue);

        Json::Value arr(Json::arrayValue);
        while (sqlite3_step(stmt.get()) == SQLITE_ROW)
            arr.append((const char *)sqlite3_column_text(stmt.get(), 0));
        return arr;
    }

} // namespace LogQuery
