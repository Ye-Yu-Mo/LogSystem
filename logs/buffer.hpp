/**
 * @file buffer.hpp
 * @brief 实现异步日志缓冲区
 *
 * 本文件定义了 Buffer 类，该类实现了一个异步日志缓冲区，用于管理日志数据的写入和读取。
 */
#pragma once
#include "util.hpp"
#include <vector>
#include <cassert>
namespace Xulog
{
// 设置默认缓冲区大小10MB，阈值缓冲区大小100MB，增量大小10MB
#define DEFAULT_BUFFER_SIZE (10 * 1024 * 1024)    ///< 默认缓冲区大小
#define THRESHOLD_BUFFER_SIZE (100 * 1024 * 1024) ///< 阈值缓冲区大小
#define INCREMENT_BUFFER_SIZE (10 * 1024 * 1024)  ///< 增量大小
    /**
     * @class Buffer
     * @brief 异步日志缓冲区类
     *
     * 该类实现了一个可扩展的缓冲区，支持数据的写入和读取操作，管理日志数据的存储。
     */
    class Buffer
    {
    public:
        /**
         * @brief 构造函数
         *
         * 创建一个新的 Buffer 对象，并初始化缓冲区大小和指针。
         */
        Buffer()
            : _buffer(DEFAULT_BUFFER_SIZE), _writer_idx(0), _reader_idx(0)
        {
        }
        /**
         * @brief 向缓冲区写入数据
         *
         * @param data 要写入的数据
         * @param len 数据的长度
         *
         * 在缓冲区中写入指定长度的数据，如果空间不足则进行扩容。
         */
        void push(const char *data, size_t len)
        {
            // 动态空间，则扩容写入
            ensureEnoughSize(len);
            // 数据写入缓冲区
            std::copy(data, data + len, &_buffer[_writer_idx]);
            // 写指针偏移
            moveWriter(len);
        }
        /**
         * @brief 获取可读数据的起始地址
         *
         * @return const char* 可读数据的起始地址
         */
        const char *begin()
        {
            return &_buffer[_reader_idx];
        }
        /**
         * @brief 获取可读数据的长度
         *
         * @return size_t 可读数据的长度
         */
        size_t readAbleSize()
        {
            // 单方向写入读出的缓冲区
            return (_writer_idx - _reader_idx);
        }
        /**
         * @brief 获取可写数据的长度
         *
         * @return size_t 可写数据的长度
         */
        size_t writeAbleSize()
        {
            // 扩容测试，总可写
            return (_buffer.size() - _writer_idx);
        }
        /**
         * @brief 移动读指针
         *
         * @param len 要移动的长度
         *
         * 更新读指针的位置，移动长度为 len。
         */
        void moveReader(size_t len)
        {
            assert(len <= readAbleSize());
            _reader_idx += len;
        }
        /**
         * @brief 重置缓冲区
         *
         * 将读写指针重置为初始状态。
         */
        void reset()
        {
            _reader_idx = 0;
            _writer_idx = 0;
        }
        /**
         * @brief 交换当前缓冲区与另一个缓冲区
         *
         * @param buffer 另一个 Buffer 对象
         *
         * 将当前缓冲区与传入的缓冲区进行交换。
         */
        void swap(Buffer &buffer)
        {
            _buffer.swap(buffer._buffer);
            std::swap(_reader_idx, buffer._reader_idx);
            std::swap(_writer_idx, buffer._writer_idx);
        }
        /**
         * @brief 判空
         *
         * @return bool 如果缓冲区为空则返回 true，否则返回 false
         */
        bool empty()
        {
            return _reader_idx == _writer_idx;
        }

    private:
        /**
         * @brief 移动写指针
         *
         * @param len 要移动的长度
         *
         * 更新写指针的位置，移动长度为 len。
         */
        void moveWriter(size_t len)
        {
            assert(len + _writer_idx <= _buffer.size());
            _writer_idx += len;
        }
        /**
         * @brief 确保缓冲区有足够的空间
         *
         * @param len 要写入的数据长度
         *
         * 如果当前可写空间不足，则根据条件扩容缓冲区。
         */
        void ensureEnoughSize(size_t len)
        {
            if (len <= writeAbleSize())
                return;
            size_t new_size = 0;
            if (_buffer.size() < THRESHOLD_BUFFER_SIZE)
                new_size = _buffer.size() * 2 + len; // 小于阈值则翻倍
            else
                new_size = _buffer.size() + INCREMENT_BUFFER_SIZE + len; // 大于阈值则线性增长
            _buffer.resize(new_size);
        }

    private:
        std::vector<char> _buffer; ///< 存储缓冲区数据的字符
        size_t _reader_idx;        ///< 可读数据指针
        size_t _writer_idx;        ///< 可写数据指针
    };
}