# C++基于多设计模式下的同步和异步日志系统

## [项目文档](https://ye-yu-mo.github.io/LogSystem/)

## 主要功能

本项目主要实现一个日志系统，主要支持以下功能

* 支持多级别的日志消息
* 支持同步日志和异步日志
* 支持可靠写入日志到控制台、文件、滚动文件中
* 支持多线程并发写入日志
* 支持扩展不同的日志落地目标地
* 支持落地到日志服务器
* 支持落地到数据库 sqlite（同步 + 异步）
* 支持使用 ini 文件配置服务器落地方式和信息
* 无锁 MPSC 异步队列，高并发下吞吐提升 2.6x
* SAFE 模式硬上限背压，防止消费滞后导致 OOM

## 项目亮点

**1. 真正的结构化异步**

异步链路从头到尾携带结构化 `LogMsg`，不是把日志格式化成字符串就丢进队列。`AsyncEntry` 同时保留格式化字符串和字段级数据，Stdout 和数据库 sink 各取所需。同类竞品（spdlog、glog）的异步模式通常只能传字节流，落库靠拼字符串。

**2. 无锁 MPSC 队列 + 背压**

异步队列从互斥锁双缓冲升级为无锁 CAS 入队，吞吐 271 万条/秒（3 线程，100 字节/条），是旧版的 2.6 倍。SAFE 模式有硬上限背压——队列满时生产者 yield 等待，杜绝消费滞后导致的无限扩容 OOM。

**3. 数据库是一等公民**

`DataBaseSink` 用 `sqlite3_prepare_v2` 参数绑定按字段落表，不是拼字符串。单引号、特殊字符不崩溃。落库后的日志可以 `SELECT * FROM logs WHERE log_level='ERROR'` —— 文件日志只能 `grep`。

**4. TCP 日志服务端**

内置 TCP 服务器 + 线程池，支持多客户端日志汇聚。通过 `ServerSink` 透明推送，客户端无需关心网络传输细节。服务端落地策略由 ini 配置文件控制。

**5. 可扩展 Sink 架构**

自定义落地目标只需继承 `LogSink` 实现 `log()` 方法。`StdoutSink`、`FileSink`、`RollSinkBySize`、`RollSinkByTime`、`DataBaseSink`、`ServerSink` 六种内置落地方式开箱即用。

**6. 全链路 gtest 回归防线**

33 条单元测试覆盖等级、格式化、无锁队列、多线程并发正确性。改一行代码，`make run` 一秒钟告诉你有没有破坏现有行为。

## 运行环境

* -lpthread
* -std=c++11（核心库）/ -std=c++17（测试，gtest 要求）
* 若要使用服务端功能需要 jsoncpp 库
* 若要使用数据库功能需要 sqlite3
* 运行测试需要 googletest（`brew install googletest` / `apt install libgtest-dev`）

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
* 默认日志器格式`%d{%y-%m-%d|%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n`
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

## 服务端使用说明

服务端启动时需要指定配置文件路径 默认配置文件在`config`文件夹中的`config.ini`文件里

```ini
# 注释section 为关闭功能 当前设置为默认设置 恢复默认可用default替代

# 服务器端口 默认为8888
[Server]
port=8888

# 标准落地配置 
# 标准落地是否打开颜色 true 表示打开 false 表示关闭
[StdoutSink] 
color=true

# 文件落地配置
# 文件位置和路径配置 支持相对和绝对地址 注意需要写出文件名
[FileSink] 
path=./log/test.log

# 滚动文件（大小）配置
# 滚动文件路径名称 后面会自动生成后缀名区分不同文件
# 滚动文件大小 单位是KB 请勿输入表达式
[RollBySize] 
path=./log/roll- 
size=1024 

# 滚动文件（时间）配置
# 滚动文件路径名称 后面会自动生成后缀名区分不同文件
# 滚动类型 支持 GAP_SECOND GAP_MINUTE GAP_HOUR GAP_DAY
[RollByTime] 
path=./log/roll- 
type=GAP_SECOND

# 数据库落地配置
# 数据库文件位置和路径配置 支持相对和绝对地址 注意需要写出文件名
[DataBaseSink] 
path=./log/log.db
```

**注意** 
1. 所有的路径需要包含文件名
2. 需要去掉某种落地方式直接注释即可
3. 数据库落地现已同时支持同步和异步日志器
4. 默认值可以使用default（尚未完整测试）


## 开发环境

* WSL (Ubuntu 22.04) / macOS
* VSCode
* g++ / gdb
* Makefile
* googletest（单元测试）

## 核心技术点

* 类的继承和多态
* C++11 多线程、智能指针、右值引用
* 无锁 MPSC 队列（`std::atomic` + CAS 无锁入队）
* 硬上限背压机制（SAFE 模式防 OOM）
* 双缓冲区 → 无锁队列演进
* 生产者消费者模型
* 多线程单元测试（gtest + 并发校验）
* 设计模式（单例模式、工厂模式、代理模式、建造者模式）
* SQLite 参数绑定（防 SQL 注入）
* 结构化异步落地（逐条 `LogMsg` 贯穿异步链路）

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

## 异步消息队列模块

### 模块设计

异步日志器的核心思想是将日志写入（磁盘 I/O）从业务线程剥离到专门的异步线程，避免业务逻辑因 IO 阻塞。

异步队列从「互斥锁 + 双缓冲字节流」升级为「无锁 MPSC 队列 + 结构化消息」。队列元素 `AsyncEntry` 同时携带：
- `LogMsg msg`：结构化字段（供 DataBaseSink 落库）
- `std::string formatted`：预格式化字符串（在生产者线程完成，供 Stdout/File 直接写出）

### 队列设计

1. **无锁 MPSC（多生产者单消费者）**：基于侵入式单向链表 + `std::atomic` CAS 入队，单一消费者 `exchange` 取全部节点后反转得 FIFO 顺序。生产者无锁竞争，消费者无需 CAS。
2. **消费者批处理**：一次取出当前队列全部元素，批量落地，减少上下文切换。
3. **SAFE 背压机制**：`atomic` 计数器跟踪队列长度，达到硬上限时生产者 `yield` 等待消费者腾空间。防止消费滞后时无限扩容 → OOM。
4. **UNSAFE 模式**：不设上限，仅用于性能基准测试。

对比旧版双缓冲设计：

| 维度 | 旧版双缓冲 | 新版无锁 MPSC |
|------|-----------|--------------|
| 锁竞争 | 所有生产者抢一把 mutex | 无锁，CAS |
| 数据形态 | 字节流 | AsyncEntry（结构化+格式化） |
| 异步落库 | 不支持（丢失 LogMsg） | 完整支持 |
| 背压 | 形同虚设（Buffer 自动扩容） | 硬上限 + yield |
| 吞吐 | 102 万/秒 | 271 万/秒 (2.6x) |

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

#### 异步（无锁 MPSC，macOS / Apple M-series）

测试命令：`cd bench && g++ bench.cc -o bench_test -std=c++14 -lpthread && ./bench_test`

| 模式 | 线程 | 条数 | 吞吐 | 带宽 |
|------|------|------|------|------|
| 异步 UNASFE | 3 | 100k × 100B | **271 万条/秒** | 258 MB/秒 |

> 对比旧版双缓冲（102 万条/秒），无锁 MPSC 队列带来 **2.6 倍**吞吐提升。

## 测试体系

`test/` 目录包含 41 条 gtest 单元测试，覆盖核心模块：

```
cd test && make run
```

| 测试文件 | 覆盖内容 |
|----------|----------|
| `test_level.cc` | LogLevel toString/fromString 往返 |
| `test_format.cc` | Formatter 全占位符 + 非法格式串异常 |
| `test_buffer.cc` | Buffer push/read/swap/reset/扩容 |
| `test_logger.cc` | SyncLogger 多线程并发 + 字段不错位 |
| `test_mpsc_queue.cc` | MPSC 无锁队列 + 背压 + 并发无损 |

## TODO

* ~~支持配置服务端地址，网络传输到日志服务器(TCP)~~ DONE
* ~~支持使用配置文件操控服务端设置~~ DONE
* ~~支持在控制台通过日志等级渲染不同颜色方便定位~~ DONE
* ~~支持日志落地到数据库~~ DONE（已支持同步 + 异步）
* ~~异步队列无锁化（MPSC + CAS 入队）~~ DONE
* ~~异步路径支持结构化落库（DataBaseSink）~~ DONE
* 实现日志器客户端，提供检索、分析、展示等功能（仅数据库）
* 支持客户端连接服务端进行检索、分析、展示等功能（仅数据库）
* 异步多 Logger 共享线程池
* 分段/无锁化进一步提升多生产者并发度
* 日志压缩与归档