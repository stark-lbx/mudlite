#include <iostream>

#include "logger.h"

using namespace mudlite;

Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

// 设置日志级别
void Logger::setLogLevel(int level) { logLevel_ = level; }

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg)
{
    // 打印级别信息
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        std::cout << "[UNKNOWN]";
        break;
    }
    // 打印时间time
    std::cout << "print time : ";
    // 打印信息msg
    std::cout << msg << std::endl;
}
