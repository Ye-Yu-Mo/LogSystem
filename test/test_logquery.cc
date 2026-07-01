// test_logquery.cc —— LogQuery 共享查询引擎测试（TDD）
#include <gtest/gtest.h>
#include "../tools/LogQuery.hpp"
#include <sqlite3.h>
#include <jsoncpp/json/json.h>
#include <set>

class LogQueryTest : public ::testing::Test
{
protected:
    sqlite3 *_db = nullptr;

    void SetUp() override
    {
        sqlite3_open(":memory:", &_db);
        const char *sql =
            "CREATE TABLE logs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "log_time TIMESTAMP NOT NULL,"
            "line_number INT,"
            "thread_id VARCHAR(255),"
            "log_level VARCHAR(10) NOT NULL,"
            "source_file VARCHAR(255),"
            "logger_name VARCHAR(255),"
            "message TEXT);";
        sqlite3_exec(_db, sql, nullptr, nullptr, nullptr);

        const char *inserts[] = {
            "INSERT INTO logs VALUES(1,datetime(1719763200,'unixepoch','+8 hours'),10,'0xAAA','ERROR','a.cc','root','timeout connecting to db')",
            "INSERT INTO logs VALUES(2,datetime(1719763260,'unixepoch','+8 hours'),20,'0xBBB','WARN','b.cc','worker','slow query detected')",
            "INSERT INTO logs VALUES(3,datetime(1719763320,'unixepoch','+8 hours'),30,'0xAAA','INFO','a.cc','root','request completed in 42ms')",
            "INSERT INTO logs VALUES(4,datetime(1719763380,'unixepoch','+8 hours'),40,'0xCCC','ERROR','c.cc','server','connection timeout')",
            "INSERT INTO logs VALUES(5,datetime(1719763440,'unixepoch','+8 hours'),50,'0xAAA','DEBUG','a.cc','root','entering function foo')",
        };
        for (auto &s : inserts)
            sqlite3_exec(_db, s, nullptr, nullptr, nullptr);
    }

    void TearDown() override { sqlite3_close(_db); }
};

TEST_F(LogQueryTest, QueryAllReturnsAllRows)     { EXPECT_EQ(5u, LogQuery::queryLogs(_db, {}).size()); }

TEST_F(LogQueryTest, QueryFilterBySingleLevel)   { LogQuery::Params p; p.level="ERROR"; auto r=LogQuery::queryLogs(_db,p); ASSERT_EQ(2u,r.size()); EXPECT_EQ("ERROR",r[0]["log_level"].asString()); }

TEST_F(LogQueryTest, QueryFilterByMultipleLevels){ LogQuery::Params p; p.level="ERROR,WARN"; EXPECT_EQ(3u, LogQuery::queryLogs(_db,p).size()); }

TEST_F(LogQueryTest, QueryFilterByKeyword)       { LogQuery::Params p; p.keyword="timeout"; auto r=LogQuery::queryLogs(_db,p); ASSERT_EQ(2u,r.size()); for(auto& x:r) EXPECT_NE(std::string::npos,x["message"].asString().find("timeout")); }

TEST_F(LogQueryTest, QueryFilterByKeywordSpecialChar)
{
    sqlite3_exec(_db, "INSERT INTO logs VALUES(6,datetime(1719763500,'unixepoch','+8 hours'),60,'0xDDD','INFO','d.cc','test','it''s a test')", nullptr, nullptr, nullptr);
    LogQuery::Params p; p.keyword="it's";
    auto r=LogQuery::queryLogs(_db,p); ASSERT_EQ(1u,r.size());
    EXPECT_NE(std::string::npos, r[0]["message"].asString().find("it's"));
}

TEST_F(LogQueryTest, QueryFilterByLogger)        { LogQuery::Params p; p.logger="worker"; auto r=LogQuery::queryLogs(_db,p); ASSERT_EQ(1u,r.size()); EXPECT_EQ("worker",r[0]["logger_name"].asString()); }

TEST_F(LogQueryTest, QueryFilterByTid)           { LogQuery::Params p; p.tid="0xAAA"; EXPECT_EQ(3u, LogQuery::queryLogs(_db,p).size()); }

TEST_F(LogQueryTest, QueryWithLimitAndOffset)    { LogQuery::Params p; p.limit=2; p.offset=1; auto r=LogQuery::queryLogs(_db,p); ASSERT_EQ(2u,r.size()); EXPECT_EQ(2,r[0]["id"].asInt()); EXPECT_EQ(3,r[1]["id"].asInt()); }

TEST_F(LogQueryTest, QueryFilterByTimeRange)     { LogQuery::Params p; p.from_time="2024-07-01 00:01"; p.to_time="2024-07-01 00:05"; EXPECT_EQ(4u, LogQuery::queryLogs(_db,p).size()); }

TEST_F(LogQueryTest, StatsByLevel)               { LogQuery::Params p; p.group_by="level"; auto r=LogQuery::logStats(_db,p); int t=0; for(auto& g:r["groups"]) t+=g["count"].asInt(); EXPECT_EQ(5,t); }

TEST_F(LogQueryTest, StatsByLogger)              { LogQuery::Params p; p.group_by="logger"; EXPECT_GE(LogQuery::logStats(_db,p)["groups"].size(), 2u); }

TEST_F(LogQueryTest, ListLoggers)                { auto r=LogQuery::listLoggers(_db); std::set<std::string> n; for(auto& x:r) n.insert(x.asString()); EXPECT_TRUE(n.count("root")); EXPECT_TRUE(n.count("worker")); }

TEST_F(LogQueryTest, EmptyDatabase)              { sqlite3* e; sqlite3_open(":memory:",&e); sqlite3_exec(e,"CREATE TABLE logs (id INTEGER PRIMARY KEY, log_time TIMESTAMP, line_number INT, thread_id VARCHAR, log_level VARCHAR, source_file VARCHAR, logger_name VARCHAR, message TEXT)",0,0,0); EXPECT_EQ(0u,LogQuery::queryLogs(e,{}).size()); sqlite3_close(e); }

TEST_F(LogQueryTest, NoMatchingResults)          { LogQuery::Params p; p.level="FATAL"; EXPECT_EQ(0u, LogQuery::queryLogs(_db,p).size()); }
