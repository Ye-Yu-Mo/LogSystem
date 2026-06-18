// mpsc_queue.hpp —— 无锁 MPSC 队列（多生产者单消费者）
//
// 设计：
// - 侵入式单向链表，生产者 CAS 竞争头节点（push front）
// - 单一消费者 exchange 头指针为 nullptr，反转链表得 FIFO 顺序
// - atomic 计数器支持 SAFE 模式背压
// - 消费者空转时短休眠，生产者零同步开销
#pragma once

#include <atomic>
#include <vector>
#include <utility>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace Xulog
{
    template <typename T>
    class MpscQueue
    {
        struct Node
        {
            T data;
            std::atomic<Node *> next{nullptr};
            Node(T &&d) : data(std::move(d)) {}
        };

    public:
        MpscQueue(size_t max_size = 0, bool safe_mode = true)
            : _max_size(max_size), _safe_mode(safe_mode) {}

        ~MpscQueue()
        {
            auto leftover = popAll();
            (void)leftover;
        }

        /// @brief 尝试入队：成功返回 true；SAFE 模式满时返回 false（调用方应重试）
        bool tryPush(const T &item)
        {
            if (_safe_mode && _count.load(std::memory_order_relaxed) >= _max_size)
                return false;

            auto *node = new Node(T(item)); // 拷贝一份，成功后移动进节点

            Node *old_head = _head.load(std::memory_order_relaxed);
            do
            {
                node->next.store(old_head, std::memory_order_relaxed);
            } while (!_head.compare_exchange_weak(old_head, node,
                                                   std::memory_order_release,
                                                   std::memory_order_relaxed));

            _count.fetch_add(1, std::memory_order_relaxed);
            _has_data.store(true, std::memory_order_release);
            return true;
        }

        /// @brief 推入队列，SAFE 模式下阻塞直到成功；UNSAFE 直接入队
        void push(const T &item)
        {
            if (_safe_mode)
            {
                while (!tryPush(item))
                    std::this_thread::yield();
            }
            else
            {
                auto *node = new Node(T(item));
                Node *old_head = _head.load(std::memory_order_relaxed);
                do
                {
                    node->next.store(old_head, std::memory_order_relaxed);
                } while (!_head.compare_exchange_weak(old_head, node,
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed));
                _count.fetch_add(1, std::memory_order_relaxed);
                _has_data.store(true, std::memory_order_release);
            }
        }

        /// @brief 消费者：一次取出全部元素（FIFO 顺序）
        std::vector<T> popAll()
        {
            Node *head = _head.exchange(nullptr, std::memory_order_acquire);
            _count.store(0, std::memory_order_relaxed);
            _has_data.store(false, std::memory_order_relaxed);

            if (!head)
                return {};

            // 反转链表，得到 FIFO 顺序
            Node *prev = nullptr;
            Node *curr = head;
            while (curr)
            {
                Node *next = curr->next.load(std::memory_order_relaxed);
                curr->next.store(prev, std::memory_order_relaxed);
                prev = curr;
                curr = next;
            }

            // prev 现在是实际队头（最早 push 的节点）
            std::vector<T> result;
            Node *node = prev;
            while (node)
            {
                result.push_back(std::move(node->data));
                Node *to_delete = node;
                node = node->next.load(std::memory_order_relaxed);
                delete to_delete;
            }

            return result;
        }

        /// @brief 队列是否为空
        bool empty() const
        {
            return _head.load(std::memory_order_relaxed) == nullptr;
        }

        /// @brief 是否有数据（比 empty 更轻量，用于消费者轮询）
        bool hasData() const
        {
            return _has_data.load(std::memory_order_acquire);
        }

        size_t count() const
        {
            return _count.load(std::memory_order_relaxed);
        }

    private:
        std::atomic<Node *> _head{nullptr};
        std::atomic<size_t> _count{0};
        std::atomic<bool> _has_data{false};
        size_t _max_size;
        bool _safe_mode;
    };

} // namespace Xulog
