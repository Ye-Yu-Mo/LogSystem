/**
 * @file looper.hpp
 * @brief 实现异步工作器
 *
 * 本文件定义了异步工作器类，用于处理异步数据生产与消费的逻辑。
 */
#pragma once
#include "buffer.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <memory>
#include <atomic>

namespace Xulog
{
    /**
     * @typedef Functor
     * @brief 回调函数类型
     *
     * 定义了一个回调函数类型，用于处理消费缓冲区的数据。
     */
    using Functor = std::function<void(Buffer &)>;
    /**
     * @enum AsyncType
     * @brief 异步工作器类型
     *
     * 定义了异步工作器的两种类型：
     * - ASYNC_SAFE: 当缓冲区满时，阻塞生产者
     * - ASYNC_UNSAFE: 不考虑资源，无限制扩容，适用于性能测试
     */
    enum class AsyncType
    {
        ASYNC_SAFE,  ///< 缓冲区满则阻塞
        ASYNC_UNSAFE ///< 不考虑资源，无限扩容，性能测试
    };
    /**
     * @class AsyncLooper
     * @brief 异步工作器类
     *
     * 该类实现了一个异步工作器，通过生产者-消费者模式处理数据。
     */
    class AsyncLooper
    {
    public:
        using ptr = std::shared_ptr<AsyncLooper>;
        /**
         * @brief 构造函数
         *
         * @param func 回调函数，用于处理消费缓冲区的数据
         * @param asynctype 异步类型，默认为 ASYNC_SAFE
         *
         * 构造 AsyncLooper 对象，并启动异步工作线程。
         */
        AsyncLooper(const Functor &func, AsyncType asynctype = AsyncType::ASYNC_SAFE)
            : _stop(false), _thread(std::thread(&AsyncLooper::threadEntry, this)), _callBack(func), _looper_type(asynctype)
        {
        }
        /**
         * @brief 析构函数
         *
         * 停止异步工作器，唤醒所有工作线程并等待线程结束。
         */
        ~AsyncLooper()
        {
            stop();
            _cond_con.notify_all(); // 唤醒所有工作线程
            _thread.join();         // 等待线程
        }
        /**
         * @brief 停止异步工作器
         *
         * 设置停止标志，并唤醒所有工作线程。
         */
        void stop()
        {
            _stop = true;
            _cond_con.notify_all(); // 唤醒所有的工作线程
        }
        /**
         * @brief 向生产缓冲区推送数据
         *
         * @param data 要推送的数据
         * @param len 数据的长度
         *
         * 根据异步类型控制缓冲区的写入，唤醒消费者处理数据。
         */
        void push(const char *data, size_t len)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 条件变量控制，若缓冲区剩余大于等于数据长度，则返回真
            if (_looper_type == AsyncType::ASYNC_SAFE)
            {
                _cond_pro.wait(lock, [&]()
                               { return _pro_buf.writeAbleSize() >= len; });
            }
            // 满足条件，添加数据
            _pro_buf.push(data, len);
            // 唤醒消费者对缓冲区的数据进行处理
            _cond_con.notify_one();
        }

    private:
        /**
         * @brief 线程入口函数
         *
         * 持续监测生产缓冲区的数据，并处理消费缓冲区的数据。
         *
         * 当停止标志被设置且缓冲区为空时，线程结束。
         */
        void threadEntry() // 线程入口函数
        {
            while (true)
            {
                // 判断生产缓冲区是否有数据，有则交换，无则阻塞
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    if (_stop && _pro_buf.empty())
                        break;
                    _cond_con.wait(lock, [&]()
                                   { return _stop || !_pro_buf.empty(); });

                    _con_buf.swap(_pro_buf);
                }
                // 处理消费缓冲区
                _callBack(_con_buf);
                // 初始化消费缓冲区
                _con_buf.reset();
                // 唤醒生产者
                if (_looper_type == AsyncType::ASYNC_SAFE)
                {
                    _cond_pro.notify_all();
                }
            }
        }

    private:
        std::atomic<bool> _stop;           ///< 停止标志
        Buffer _pro_buf;                   ///< 生产缓冲区
        Buffer _con_buf;                   ///< 消费缓冲区
        std::mutex _mutex;                 ///< 互斥锁
        std::condition_variable _cond_pro; ///< 生产者条件变量
        std::condition_variable _cond_con; ///< 消费者条件变量
        std::thread _thread;               ///< 异步工作器的线程
        AsyncType _looper_type;            ///< 异步类型
        Functor _callBack;                 ///< 回调函数
    };
}