# LogSystem
Synchronous&amp;Asynchronous Blog System Based on Multiple Design Patterns By C++

# C++基于多设计模式下的同步和异步日志系统

## 主要功能

本项目主要实现一个日志系统，主要支持以下功能
* 支持多级别的日志消息
* 支持同步日志和异步日志
* 支持可靠写入日志到控制台、文件、滚动文件中
* 支持多线程并发写入日志
* 支持扩展不同的日志落地目标地

## 核心技术点

* 类的继承和多态
* C++11多线程、智能指针、右值引用
* 双缓冲区
* 生产者消费者模型
* 多线程
* 设计模式（单例模式、工厂模式、代理模式、建造者模式）