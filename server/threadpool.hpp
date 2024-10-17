/// @file threadpool
/// @brief 线程池
#pragma once
#include <iostream>
#include <functional>
#include <memory>
#include <thread>
#include <future>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <atomic>
#include "../logs/Xulog.h"

namespace XuServer
{
    /// @class threadpool
    /// @brief 线程池类
    class threadpool
    {
    public:
        using ptr = std::unique_ptr<threadpool>;   ///< 线程池操作句柄
        using Functor = std::function<void(void)>; ///< 线程池回调函数

        /// @brief 构造函数
        /// @param thr_count 线程数量
        threadpool(int thr_count = 1) : _stop(false)
        {
            for (int i = 0; i < thr_count; i++)
                _threads.emplace_back(&threadpool::entry, this);
        }
        /// @brief 析构函数
        ~threadpool()
        {
            stop();
        }
        /// @brief 停止所有线程
        void stop()
        {
            if (_stop == true)
                return;
            _stop = true;
            _cv.notify_all(); // 唤醒线程
            for (auto &thread : _threads)
                thread.join();
        }

        /// @brief 传入任务函数到任务池
        /// @tparam F 任务函数类型
        /// @tparam ...Args 任务函数参数包类型
        /// @param func 任务函数
        /// @param ...args 任务函数参数包
        /// @return 自动推导任务函数返回值类型
        template <typename F, typename... Args>
        auto push(F &&func, Args &&...args) -> std::future<decltype(func(args...))>
        {
            // 将传入函数封装成packaged_task任务包
            using return_type = decltype(func(args...));
            auto tmp_func = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
            auto task = std::make_shared<std::packaged_task<return_type()>>(tmp_func);
            std::future<return_type> fu = task->get_future();
            // 构造lambda表达式(捕获任务对象,函数内执行任务对象)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                // 将构造出来的匿名对象传入任务池
                _taskpool.push_back([task]()
                                    { (*task)(); });
                _cv.notify_one();
            }
            return fu;
        }

    private:
        /// @brief 线程入口函数 从任务池中取出任务执行
        void entry()
        {
            while (!_stop)
            {
                // 临时任务池
                // 避免频繁加解锁
                std::vector<Functor> tmp_taskpool;
                {
                    // 加锁
                    std::unique_lock<std::mutex> lock(_mutex);
                    // 等待任务不为空或_stop被置为1
                    _cv.wait(lock, [this]()
                             { return _stop || !_taskpool.empty(); });

                    // 取出任务进行执行
                    tmp_taskpool.swap(_taskpool);
                }
                for (auto &task : tmp_taskpool)
                {
                    task();
                }
            }
        }

    private:
        std::atomic<bool> _stop;           ///< 原子类型的停止标志
        std::vector<Functor> _taskpool;    ///< 任务池
        std::mutex _mutex;                 ///< 互斥锁
        std::condition_variable _cv;       ///< 条件变量
        std::vector<std::thread> _threads; ///< 管理线程
    };
}