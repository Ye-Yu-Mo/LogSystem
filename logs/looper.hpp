/**
 * @file looper.hpp
 * @brief 异步实体 + 无锁异步工作器
 *
 * 无锁 MPSC 队列存 AsyncEntry（结构化 + 格式化双形态）
 */
#pragma once

#include "mpsc_queue.hpp"
#include "message.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include <vector>

namespace Xulog
{
    /// @brief 异步队列元素：携带结构化 LogMsg + 预格式化字符串
    struct AsyncEntry
    {
        LogMsg msg;             ///< 结构化字段（DataBaseSink 用）
        std::string formatted;  ///< 预格式化字符串（Stdout/File 用，生产者线程完成格式化）
    };

    /// @brief 消费者回调类型：一次处理一批 AsyncEntry
    using BatchCallback = std::function<void(std::vector<AsyncEntry> &)>;

    /// @brief 异步工作器类型
    enum class AsyncType
    {
        ASYNC_SAFE,   ///< 安全模式：队列有硬上限，满时生产者背压阻塞
        ASYNC_UNSAFE  ///< 非安全模式：不设上限，仅性能测试用
    };

    /// @brief 默认 SAFE 模式队列最大容量
    constexpr size_t DEFAULT_MAX_QUEUE_SIZE = 1024 * 256; // 26w 条，约 25MB

    /**
     * @class AsyncLooper
     * @brief 无锁 MPSC 异步工作器
     *
     * 生产者（业务线程）：CAS 入队 AsyncEntry，无锁
     * 消费者（单一线程）：批量取出，调用回调逐条落地
     * SAFE 模式：队列有硬上限，生产快于消费时自动背压（yield 等待）
     */
    class AsyncLooper
    {
    public:
        using ptr = std::shared_ptr<AsyncLooper>;

        AsyncLooper(const BatchCallback &func,
                    AsyncType asynctype = AsyncType::ASYNC_SAFE,
                    size_t max_queue = DEFAULT_MAX_QUEUE_SIZE)
            : _queue(max_queue, asynctype == AsyncType::ASYNC_SAFE),
              _looper_type(asynctype),
              _callBack(func),
              _stop(false),
              _thread(std::thread(&AsyncLooper::threadEntry, this))
        {
        }

        ~AsyncLooper()
        {
            stop();
            // 最后唤醒一次确保 _thread 退出
            {
                std::lock_guard<std::mutex> lk(_sleep_mutex);
            }
            _sleep_cv.notify_one();
            if (_thread.joinable())
                _thread.join();
        }

        void stop()
        {
            _stop.store(true, std::memory_order_release);
            {
                std::lock_guard<std::mutex> lk(_sleep_mutex);
            }
            _sleep_cv.notify_one();
        }

        /// @brief 生产者入队（无锁 CAS，SAFE 模式下满则 yield 等待）
        void push(AsyncEntry &&entry)
        {
            _queue.push(std::move(entry));
        }

        /// @brief 获取当前队列长度（用于监控）
        size_t queueSize() const { return _queue.count(); }

    private:
        void threadEntry()
        {
            while (true)
            {
                auto batch = _queue.popAll();
                if (!batch.empty())
                {
                    _callBack(batch);
                    continue;
                }
                // 队列空：检查退出，否则短休眠避免空转
                if (_stop.load(std::memory_order_acquire))
                    break;
                std::unique_lock<std::mutex> lk(_sleep_mutex);
                _sleep_cv.wait_for(lk, std::chrono::milliseconds(1));
            }
        }

        MpscQueue<AsyncEntry> _queue;    ///< 无锁 MPSC 队列
        AsyncType _looper_type;          ///< 异步类型
        BatchCallback _callBack;         ///< 消费者回调
        std::atomic<bool> _stop;         ///< 停止标志
        std::thread _thread;             ///< 消费者线程
        std::mutex _sleep_mutex;         ///< 休眠锁（仅消费者侧）
        std::condition_variable _sleep_cv; ///< 休眠条件变量（仅消费者侧）
    };

} // namespace Xulog
