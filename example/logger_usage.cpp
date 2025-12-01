// example/logger_usage.cpp
#include "logger/logger.hpp"

// 使用Writer Monad
logger::Logged<int> compute_with_logging(int x) {
    return logger::pure(x)
        .log_info("Starting computation")
        .map([](int v) { return v * 2; })
        .log_info("Doubled the value")
        .map([](int v) { return v + 10; })
        .log_info("Added 10");
}

int main() {
    // 示例1: 简单的控制台和文件日志
    auto simple_logger = logger::Logger(logger::Level::Debug)
        .with_sink(std::make_shared<logger::ConsoleSink>())
        .with_sink(std::make_shared<logger::FileSink>("server.log"));

    // 示例2: 使用轮转文件（按日期和行数自动轮转）
    auto rotating_logger = logger::Logger(logger::Level::Info)
        .with_sink(std::make_shared<logger::ConsoleSink>())
        .with_sink(std::make_shared<logger::RotatingFileSink>(
            "./logs",           // 日志目录
            "server",           // 基础文件名
            5000000             // 最大行数（默认500万行）
        ));

    // 示例3: 异步日志（推荐用于生产环境）
    auto async_logger = logger::Logger(logger::Level::Debug)
        .with_sink(std::make_shared<logger::ConsoleSink>())
        .with_sink(std::make_shared<logger::AsyncSink>(
            std::make_unique<logger::RotatingFileSink>(
                "./logs",
                "async_server",
                1000000         // 100万行轮转
            )
        ));

    // 普通日志
    LOG_INFO(async_logger, "Server starting");
    LOG_ERROR(async_logger, "Connection failed");

    // Writer Monad模式
    auto result = compute_with_logging(5);
    async_logger.write_logged(result);  // 统一写出所有累积的日志
    std::cout << "Final result: " << result.value() << "\n";

    async_logger.flush();
    return 0;
}
