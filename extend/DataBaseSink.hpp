/**
 * @file DataBasesSink.hpp
 * @brief 定义了 SqliteHelper DataBaseSink类，用于数据库操作和落地到数据库
 */
#pragma once
#include "../logs/util.hpp"
#include "../logs/Xulog.h"
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sqlite3.h>

/**
 * @class SqliteHelper
 * @brief SQLite 数据库操作助手类
 *
 * 此类封装了 SQLite 数据库的基本操作，包括创建/打开数据库、
 * 执行 SQL 语句和关闭数据库的功能。
 *
 * 主要功能：
 * - 创建和打开数据库
 * - 执行 SQL 语句（包括表操作和数据操作）
 * - 关闭数据库
 */
class SqliteHelper
{
public:
    /**
     * @typedef SqliteCallback
     * @brief SQLite 回调函数类型
     *
     * 回调函数用于处理 SQLite 执行 SQL 语句后的结果。
     *
     * @param arg 用户自定义参数
     * @param count 返回的列数
     * @param values 返回的列值
     * @param names 返回的列名
     * @return 整数值，表示处理结果
     */
    typedef int (*SqliteCallback)(void *, int, char **, char **);
    /**
     * @brief SqliteHelper 构造函数
     * @param dbfile 数据库文件路径
     *
     * 使用给定的数据库文件路径初始化 SqliteHelper 对象。
     */
    SqliteHelper(const std::string &dbfile)
        : _dbfile(dbfile), _handler(nullptr)
    {
    }
    /**
     * @brief 打开数据库
     * @param safe_level 线程安全级别（默认为 SQLITE_OPEN_FULLMUTEX）
     * @return 成功返回 true，失败返回 false
     *
     * 该方法尝试打开指定的 SQLite 数据库，如果失败则记录错误日志。
     */
    bool open(int safe_level = SQLITE_OPEN_FULLMUTEX)
    {
        int ret = sqlite3_open_v2(_dbfile.c_str(), &_handler, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | safe_level, nullptr);
        if (ret != SQLITE_OK)
        {
            std::cout << "打开数据库 " << _dbfile.c_str() << " 失败!" << std::endl;
            return false;
        }
        return true;
    }
    /**
     * @brief 执行 SQL 语句
     * @param sql 要执行的 SQL 语句
     * @param cb 回调函数，用于处理结果
     * @param arg 用户自定义参数
     * @return 成功返回 true，失败返回 false
     *
     * 该方法执行指定的 SQL 语句，如果失败则记录错误日志。
     */
    bool exec(const std::string &sql, SqliteCallback cb, void *arg)
    {
        int ret = sqlite3_exec(_handler, sql.c_str(), cb, arg, nullptr);
        if (ret != SQLITE_OK)
        {
            std::cout << sql.c_str() << "--执行语句失败: " << sqlite3_errmsg(_handler) << " 失败!" << std::endl;
            return false;
        }
        return true;
    }
    /**
     * @brief 关闭数据库
     *
     * 该方法关闭打开的 SQLite 数据库。
     */
    void close()
    {
        sqlite3_close_v2(_handler);
    }

private:
    std::string _dbfile; ///< 数据库文件路径
    sqlite3 *_handler;   ///< SQLite 数据库句柄
};

/// @brief 数据库落地类
class DataBaseSink : public Xulog::LogSink
{
public:
    using DBptr = std::shared_ptr<DataBaseSink>; ///< 数据库落地操作句柄
    /// @brief 数据库落地类构造函数
    /// @param dbfile 数据库文件路径
    /// @param name 日志器名称
    DataBaseSink(const std::string &dbfile, const std::string &name)
        : _helper(dbfile), _logger_name(name)
    {
        if (!Xulog::Util::File::exists(dbfile))
        {
            std::string path = Xulog::Util::File::path(dbfile);
            Xulog::Util::File::createDirectory(path);
            Xulog::Util::File::createFile(dbfile);
        }
        assert(_helper.open());
        createTable();
    }
    /// @brief 用于创建日志表
    void createTable()
    {
        const char *CREATE_TABLE = "CREATE TABLE IF NOT EXISTS logs (id INTEGER PRIMARY KEY AUTOINCREMENT, log_time TIMESTAMP NOT NULL,\
                                    line_number INT, thread_id VARCHAR(255), log_level VARCHAR(10) NOT NULL,\
                                    source_file VARCHAR(255), logger_name VARCHAR(255),message TEXT);";
        bool ret = _helper.exec(CREATE_TABLE, nullptr, nullptr);
        if (ret == false)
        {
            ERROR("创建日志数据库表失败!");
            abort();
        }
    }
    /// @brief 存储到数据库
    /// @param data 数据指针
    /// @param len 数据长度
    void log(const char *data, size_t len)
    {
        if (_logger == nullptr)
            _logger = Xulog::getLogger(_logger_name);
        if (_logger == nullptr)
        {
            std::cout << "未获取到日志器！日志器名称为：" << _logger_name << std::endl;
        }
        Xulog::LogMsg msg = _logger->getMsg();
        std::stringstream ss;
        ss << "INSERT into logs (log_time, line_number, thread_id, log_level, source_file, logger_name, message) ";
        ss << "VALUES (datetime(" << msg._ctime << ", 'unixepoch', '+8 hours'),";
        ss << msg._line << ", ";
        ss << "'" << msg._tid << "', ";
        ss << "'" << Xulog::LogLevel::toString(msg._level) << "', ";
        ss << "'" << msg._file << "', ";
        ss << "'" << msg._logger << "', ";
        ss << "'" << msg._payload << "');";
        bool ret = _helper.exec(ss.str().c_str(), nullptr, nullptr);
        if (ret == false)
        {
            ERROR("插入日志数据失败!");
            abort();
        }
    }
    /// @brief 存储到数据库
    /// @param msg 格式化的数据
    void log(const Xulog::LogMsg &msg)
    {
        std::stringstream ss;
        ss << "INSERT into logs (log_time, line_number, thread_id, log_level, source_file, logger_name, message) ";
        ss << "VALUES (datetime(" << msg._ctime << ", 'unixepoch', '+8 hours'),";
        ss << msg._line << ", ";
        ss << "'" << msg._tid << "', ";
        ss << "'" << Xulog::LogLevel::toString(msg._level) << "', ";
        ss << "'" << msg._file << "', ";
        ss << "'" << msg._logger << "', ";
        ss << "'" << msg._payload << "');";
        bool ret = _helper.exec(ss.str().c_str(), nullptr, nullptr);
        if (ret == false)
        {
            ERROR("插入日志数据失败!");
            abort();
        }
    }
    ~DataBaseSink() {}

private:
    SqliteHelper _helper;              ///< 数据库帮助类句柄
    std::string _logger_name;          ///< 日志器名称
    Xulog::LogMsg _msg;                ///< 结构化数据
    static Xulog::Logger::ptr _logger; ///< 日志器句柄
};
Xulog::Logger::ptr DataBaseSink::_logger = nullptr; ///< 初始化日志器句柄
