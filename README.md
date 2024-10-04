# C++基于多设计模式下的同步和异步日志系统

## 主要功能

本项目主要实现一个日志系统，主要支持以下功能

* 支持多级别的日志消息
* 支持同步日志和异步日志
* 支持可靠写入日志到控制台、文件、滚动文件中
* 支持多线程并发写入日志
* 支持扩展不同的日志落地目标地

## 运行环境

* -lpthread
* -std=c++11

## 使用方法

包含`./logs/Xulog.h`即可

### 默认日志器

```cpp
DEBUG("%s", "测试");
INFO("%s", "测试");
WARN("%s", "测试");
ERROR("%s", "测试");
FATAL("%s", "测试");
```

* 默认日志器名称`root`

* 默认日志等级`DEBUG`
* 默认为同步日志器
* 默认为标准控制台输出,不显示日志等级颜色

### 自定义日志器

1. 创建日志建造器

| 接口                         | 功能                 | 选项 | 返回值类型             |
| ---------------------------- | -------------------- | ---- | ---------------------- |
| `Xulog::GlobalLoggerBuild()` | 构建全局日志器建造器 | -    | `Xulog::LoggerBuilder` |
| `Xulog::LocalLoggerBuild()`  | 构建局部日志器建造器 | -    | `Xulog::LoggerBuilder` |

```cpp
std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
```

2. 初始化日志建造器

| 接口                 | 功能           | 选项                                                         | 说明                                                         |
| -------------------- | -------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| `buildLoggerName()`  | 设定日志器名称 | 传入string即可                                               | **名称不能为空**,不可缺省                                    |
| `buildFormatter()`   | 设定日志器格式 | 见日志器格式表                                               | **默认日志器格式为 `%d{%y-%m-%d\|%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n`**,可缺省 |
| `buildLoggerLevel()` | 设定日志器等级 | `Xulog::LogLevel::value::DEBUG`<br />`Xulog::LogLevel::value::INFO`<br />`Xulog::LogLevel::value::WARN`<br />`Xulog::LogLevel::value::ERROR`<br />`Xulog::LogLevel::value::FATAL` | 只有大于等于该等级的日志被输出`DEBUG < INFO < WARN < ERROR < FATAL`，另外有`OFF`选项，表示关闭日志输出<br />**默认为DEBUG**,可缺省 |
| `buildLoggerType()`  | 设定日志器类型 | `Xulog::LoggerType::LOGGER_SYNC`<br />`Xulog::LoggerType::LOGGER_ASYNC` | `LOGGER_SYNC`表示同步日志器<br />`LOGGER_ASYNC`表示异步日志器，关于同步日志器和异步日志器见后面的介绍<br />**默认为同步日志器**,可缺省 |
| `buildSink<>()`      | 设置落地方法   | `<Xulog::StdoutSink>(Xulog::StdoutSink::Color::Enable)`<br />`<Xulog::FileSink>("file_path")`<br />`<Xulog::RollSinkBySize>("file_path-", file_size)` | 标准落地为控制台输出,传入`Xulog::StdoutSink::Color::Enable`则可以开启日志等级颜色,`Uneable`则为关闭,不建议开启,输出效率降低非常多<br />文件落地为输出到指定路径的文件中<br />以文件大小滚动落地，自带文件标号<br />可扩展至远程日志服务器和数据库，在extend中扩展了以时间滚动落地<br />**默认为控制台输出 关闭颜色显示** |
| build()              | 构建日志器     | -                                                            | 返回值类型为`Logger::ptr`日志器指针                          |

```cpp
    builder->buildLoggerLevel(Xulog::LogLevel::value::DEBUG);
    builder->buildLoggerName("Synclogger");
    builder->buildFormatter("[%d{%y-%m-%d|%H:%M:%S}][%c][%f:%l][%p]%T%m%n");
    builder->buildLoggerType(Xulog::LoggerType::LOGGER_SYNC);
    builder->buildSink<Xulog::StdoutSink>(true);
    builder->buildSink<Xulog::FileSink>("./log/a_test.log");
    builder->buildSink<Xulog::RollSinkBySize>("./log/a_roll-", 1024 * 1024);
    builder->build();
```

**若不需要更改，可以使用默认设置，不用进行单独调用**

3. 获取全局日志器

| 接口                     | 功能                                                         |
| ------------------------ | ------------------------------------------------------------ |
| `Xulog::getLogger(name)` | 获取指定名称的日志器，返回值为日志器指针，返回值类型为`Logger::ptr` |
| `Xulog::rootLogger()`    | 获取默认日志器，返回值为默认日志器指针，返回值类型为`Logger::ptr` |

4. 日志输出

| 接口                         | 说明                                                         |
| ---------------------------- | ------------------------------------------------------------ |
| ` DEBUG("%s", "测试开始")`   | 大写为默认全局日志器输出，此处的格式与C/C++格式化输出相同，与等级相同`DEBUG\|INFO\|WARN\|ERROR\|FATAL` |
| `debug(logger,"%s", "测试")` | 小写为指定日志器输出，需要传入日志器指针，`debug\|info\|warn\|error\|fatal` |

```cpp
debug(logger, "%s", "测试");
info(logger, "%s", "测试");
error(logger, "%s", "测试");
warn(logger, "%s", "测试");
fatal(logger, "%s", "测试");
```

**日志器格式表**

| 占位符 | 说明                                                         |
| ------ | ------------------------------------------------------------ |
| `%d`   | 日期，子格式`{%y-%m-%d\|%H:%M:%S}`年-月-日\|时-分-秒，子格式需要使用大括号 |
| `%T`   | Tab缩进                                                      |
| `%t`   | 线程ID                                                       |
| `%p`   | 日志级别 `DEBUG < INFO < WARN < ERROR < FATAL` 另外有 `OFF`选项 |
| `%c`   | 日志器名称                                                   |
| `%f`   | 文件名                                                       |
| `%l`   | 行号                                                         |
| `%m`   | 日志消息                                                     |
| `%n`   | 换行                                                         |

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
  * 格式化内容：`[%d{%y-%m%d\|%H:%M:%S}]%T[%t]%T[%p]%T[%c]%T%f:%l%T%m%n`
  * `[2024-9-20\|12:01:54]    [(TID)]    [FATAL]    [root]    main.cc:13    套接字创建失败\n`
* 日志落地模块：负责对日志指定方向的写入输出
* 日志器模块：对上面模块的整合、日志输出等级、日志信息格式化、日志消息落地模块
  * 同步日志器模块
  * 异步日志器模块
* 异步线程模块：负责异步日志落地
* 单例日志器管理模块：对日志全局管理，在项目任何位置，获取日志器执行输出

### 模块关系图

![image.png](https://s2.loli.net/2024/09/06/OJXmjhyqEwPRsbY.png)

## 异步缓冲区模块

### 模块设计

异步日志器的思想是为了避免业务线程因为写日志的过程时间较长而长时间阻塞

异步日志器的工作就是把业务输出的日志内容放入内存缓冲区中，使用专门的线程进行日志写入

这个模块的主要内容是

1. 实现一个线程安全缓冲区
2. 创建一个异步工作线程，专门负责缓冲区日志消息的落地操作

### 缓冲区设计

1. 使用队列缓存日志消息，进行逐条处理，要求不涉及到空间的频繁申请和释放，否则会降低效率
2. 使用环形队列，提前将空间申请号，然后对空间循环利用
3. 缓冲区会涉及多线程，需要保证线程安全
4. 写日志操作只需要一个线程，涉及到的锁冲突是生产者与生产者的互斥，生产者和消费者的互斥，冲突较为严重
5. 双缓冲区思想，第一个为任务写入的缓冲区，第二个是任务处理缓冲区

![image.png](https://s2.loli.net/2024/09/12/AS6YXdRypnCPxIg.png)

双缓冲区的好处是，降低了生产者和消费者之间的冲突，只有在交换的适合需要冲突一次

## 性能测试

### 测试环境

* CPU : AMD Ryzen 7 6800H with Radeon Graphics 3.20 GHz
* RAM : 16G DDR5 6400 MHz
* ROM : 512G-SSD PCIe4.0
* OS : Ubuntu 22.04.4 LTS (WSL虚拟机)  
  * 16 核（每个核心 2 个线程，8 个物理核心） 总内存 (Mem): 6.6 GiB

### 测试方法

主要测试内容：单线程 | 多线程 & 同步 | 异步

* 100w+条指定长度日志输出所耗时间
* 每秒输出日志数
* 每秒输出日志所占存储空间


### 测试结果

#### 同步

* 单线程

![_C97ADC80-004F-4A6E-B8E0-53B79C5CB19B_.png](https://s2.loli.net/2024/09/20/5wYjBKIiJup3akz.png)

* 多线程

![image.png](https://s2.loli.net/2024/09/20/TPDJBNcVKR5zXCU.png)

#### 异步

* 单线程

![image.png](https://s2.loli.net/2024/09/20/aAMf7DoVW1zImwh.png)

* 多线程

![image.png](https://s2.loli.net/2024/09/20/AJeUbpSPjDFRofr.png)

## TODO

* 支持配置服务端地址，网络传输到日志服务器（TCP/UDP）
* ~~支持在控制台通过日志等级渲染不同颜色方便定位~~
* 支持按照日志等级落地文件
* 支持落地到数据库
* 实现日志器服务端，提供检索、分析、展示等功能