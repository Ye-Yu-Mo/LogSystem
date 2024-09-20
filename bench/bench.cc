#include "../logs/Xulog.h"
#include <vector>
#include <thread>
#include <chrono>
void bench(const std::string &logger_name, size_t thr_count, size_t msg_count, size_t msg_len)
{
    // 获取日志器
    Xulog::Logger::ptr logger = Xulog::getLogger(logger_name);
    if (logger.get() == nullptr)
    {
        return;
    }
    // 提示信息
    std::cout << "测试日志 " << msg_count << " 条\t总大小 " << (msg_count * msg_len) / 1024 << " KB" << std::endl;
    // 组织指定长度的日志消息
    std::string msg(msg_len - 1, 'X'); // 末尾添加换行
    // 创建指定数量的线程
    std::vector<std::thread> threads;
    std::vector<double> cost_array(thr_count);
    size_t msg_per_thr = msg_count / thr_count;
    for (int i = 0; i < thr_count; i++)
    {
        threads.emplace_back([&, i]()
                             {
                                // 开始计时
                                auto start = std::chrono::high_resolution_clock::now();
                                // 循环写日志
                                for (int j = 0; j < msg_per_thr; j++)
                                {
                                    fatal(logger, "%s", msg);
                                }
                                // 结束计时
                                auto end = std::chrono::high_resolution_clock::now();
                                // 计算输出每个线程耗时
                                std::chrono::duration<double> cost = end-start;
                                cost_array[i] = cost.count();
                                std::cout<<"[线程-"<<i<<"]# "<<"输出日志数量:"<<msg_per_thr<< " 耗时:"<<cost.count()<<"s"<<std::endl; });
    }
    for (int i = 0; i < thr_count; i++)
    {
        threads[i].join();
    }
    // 计算总耗时 总时间 = max(time_thr);
    double max_cost = cost_array[0];
    for (int i = 0; i < thr_count; i++)
        max_cost = cost_array[i] > max_cost ? cost_array[i] : max_cost;
    size_t msg_per_sec = msg_count / max_cost;
    size_t size_per_sec = (msg_count * msg_len) / (max_cost * 1024 * 1024); // MB
    // 输出结果
    std::cout << "每秒输出日志数量: " << msg_per_sec << " 条\n";
    std::cout << "每秒输出日志大小: " << size_per_sec << " MB\n";
}
void sync_bench()
{
    std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
    builder->buildLoggerName("Synclogger");
    builder->buildFormatter("%m%n");
    // builder->buildEnableUnsafeAsync();
    builder->buildLoggerType(Xulog::LoggerType::LOGGER_SYNC);
    builder->buildSink<Xulog::FileSink>("./log/Sync.log");
    builder->build();
    bench("Synclogger", 1, 100 * 1000, 100);
}
void async_bench()
{
    std::unique_ptr<Xulog::LoggerBuilder> builder(new Xulog::GlobalLoggerBuild());
    builder->buildLoggerName("Asynclogger");
    builder->buildFormatter("%m%n");
    builder->buildEnableUnsafeAsync();
    builder->buildLoggerType(Xulog::LoggerType::LOGGER_ASYNC);
    builder->buildSink<Xulog::FileSink>("./log/Async.log");
    builder->build();
    bench("Asynclogger", 3, 100 * 1000, 100);
}
int main()
{
    async_bench();
    return 0;
}