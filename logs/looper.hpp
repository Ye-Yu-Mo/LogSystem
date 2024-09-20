/*
    实现异步工作器
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
    using Functor = std::function<void(Buffer &)>;
    enum class AsyncType
    {
        ASYNC_SAFE,  // 缓冲区满则阻塞
        ASYNC_UNSAFE // 不考虑资源，无限扩容，性能测试
    };
    class AsyncLooper
    {
    public:
        using ptr = std::shared_ptr<AsyncLooper>;
        AsyncLooper(const Functor &func, AsyncType asynctype = AsyncType::ASYNC_SAFE)
            : _stop(false), _thread(std::thread(&AsyncLooper::threadEntry, this)), _callBack(func), _looper_type(asynctype)
        {
        }
        ~AsyncLooper()
        {
            stop();
            _cond_con.notify_all(); // 唤醒所有工作线程
            _thread.join();         // 等待线程
        }
        void stop()
        {
            _stop = true;
            _cond_con.notify_all(); // 唤醒所有的工作线程
        }
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
        std::atomic<bool> _stop; // 停止标志
        Buffer _pro_buf;         // 生产缓冲区
        Buffer _con_buf;         // 消费缓冲区
        std::mutex _mutex;
        std::condition_variable _cond_pro;
        std::condition_variable _cond_con;
        std::thread _thread; // 异步工作器的线程
        AsyncType _looper_type;
        Functor _callBack; // 回调函数
    };
}