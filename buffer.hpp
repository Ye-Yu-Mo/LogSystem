/*
    实现异步日志缓冲区
*/
#pragma once
#include "util.hpp"
#include <vector>
#include <cassert>
namespace Xulog
{
// 设置默认缓冲区大小10MB，阈值缓冲区大小100MB，增量大小10MB
#define DEFAULT_BUFFER_SIZE (10 * 1024 * 1024)
#define THRESHOLD_BUFFER_SIZE (100 * 1024 * 1024)
#define INCREMENT_BUFFER_SIZE (10 * 1024 * 1024)
    class Buffer
    {
    public:
        Buffer()
            : _buffer(DEFAULT_BUFFER_SIZE), _writer_idx(0), _reader_idx(0)
        {
        }
        // 向缓冲区写入数据
        void push(const char *data, size_t len)
        {
            // 动态空间，则扩容写入
            ensureEnoughSize(len);
            // 数据写入缓冲区
            std::copy(data, data + len, &_buffer[_writer_idx]);
            // 写指针偏移
            moveWriter(len);
        }
        // 返回可读数据的起始地址
        const char *begin()
        {
            return &_buffer[_reader_idx];
        }
        // 返回可读数据的长度
        size_t readAbleSize()
        {
            // 单方向写入读出的缓冲区
            return (_writer_idx - _reader_idx);
        }
        // 返回可写的数据长度
        size_t writeAbleSize()
        {
            // 扩容测试，总可写
            return (_buffer.size() - _writer_idx);
        }
        // 读指针的偏移操作
        void moveReader(size_t len)
        {
            assert(len <= readAbleSize());
            _reader_idx += len;
        }
        // 重写读写位置，初始化
        void reset()
        {
            _reader_idx = 0;
            _writer_idx = 0;
        }
        // 交换
        void swap(Buffer &buffer)
        {
            _buffer.swap(buffer._buffer);
            std::swap(_reader_idx, buffer._reader_idx);
            std::swap(_writer_idx, buffer._writer_idx);
        }
        // 判空
        bool empty()
        {
            return _reader_idx == _writer_idx;
        }

    private:
        // 写指针的偏移操作
        void moveWriter(size_t len)
        {
            assert(len + _writer_idx <= _buffer.size());
            _writer_idx += len;
        }
        // 扩容操作
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
        std::vector<char> _buffer;
        size_t _reader_idx; // 可读数据指针
        size_t _writer_idx; // 可写数据指针
    };
}