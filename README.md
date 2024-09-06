# C++基于多设计模式下的同步和异步日志系统

## 主要功能

本项目主要实现一个日志系统，主要支持以下功能
* 支持多级别的日志消息
* 支持同步日志和异步日志
* 支持可靠写入日志到控制台、文件、滚动文件中
* 支持多线程并发写入日志
* 支持扩展不同的日志落地目标地

## 开发环境

* WSL(Ubuntu 22.04)
* VSCode
* g++/gdb
* Makefile

## 核心技术点

* 类的继承和多态
* C++11多线程、智能指针、右值引用
* 双缓冲区
* 生产者消费者模型
* 多线程
* 设计模式（单例模式、工厂模式、代理模式、建造者模式）

## 日志系统介绍

在生产环境中，有时候是不允许我们程序员利用调试器排查问题，不允许服务暂停

在高频操作中，少量调试次数并不一定能够复现出对应的bug，可能需要重复操作非常多次的情况，导致效率低下

在分布式、多线程代码中，bug更难以定位

因此就需要日志系统进行开发问题的排查

## 技术实现

日志系统的技术实现主要分为两大类

1. 输出日志到控制台

2. 输出日志到文件或数据库系统

这里分为同步和异步写日志

### 同步日志

同步日志是指当输出日志时，程序必须等待日志输出语句执行完毕后才能执行后面的逻辑语句，日志输出语句和业务逻辑处于同一个线程

同步日志系统在高并发场景下容易出现系统瓶颈，主要是由于write系统调用和频繁IO导致的

### 异步日志

异步日志指的是在输出日志时，输出日志的语句和业务逻辑语句处于不同的线程，使用单独的线程去完成

业务逻辑语句是生产者，而日志线程是消费者，这是一共典型的生产者消费者模型

异步的好处就是日志没有完成输出也不会影响业务逻辑的运行，可以提高程序的效率

## 框架模块

* 日志等级模块：对不同日志进行等级划分、对日志的输出进行控制，提供按照等级枚举字符串的功能
  * OFF：关闭
  * DEBUG：调试，调试信息输出
  * INFO：提示，提示型日志信息
  * WARN：警告，不影响运行，但可能不安全
  * ERROR：错误，程序运行出错的日志
  * FATAL：致命，代码异常
* 日志消息模块：日志中所含的各类信息
  * 时间
  * 线程ID
  * 日志等级
  * 日志数据
  * 日志文件名
  * 日志行号
* 消息格式化模块：设置日志的输出格式，提供对消息格式化的功能
  * 格式化内容：[%d{%H:%M:%S}]%T[%t]%T[%p]%T[%c]%T%f:%l%T%m%n
  * [12:01:54]    [(TID)]    [FATAL]    [root]    main.cc:13    套接字创建失败\n
* 日志落地模块：负责对日志指定方向的写入输出
* 日志器模块：对上面模块的整合、日志输出等级、日志信息格式化、日志消息落地模块
  * 同步日志器模块
  * 异步日志器模块
* 异步线程模块：负责异步日志落地
* 单例日志器管理模块：对日志全局管理，在项目任何位置，获取日志器执行输出

### 模块关系图

![image.png](https://s2.loli.net/2024/09/06/OJXmjhyqEwPRsbY.png)
