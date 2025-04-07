#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMutex>
#include <QDir>
#include <QCoreApplication>

/**
 * @brief 统一的日志管理系统类
 * 
 * 提供全局统一的日志记录功能，支持不同级别的日志，可以输出到控制台和文件
 */
class Logger {
public:
    /**
     * @brief 日志级别枚举
     */
    enum LogLevel {
        Debug,    // 调试信息，开发时使用
        Info,     // 普通信息
        Warning,  // 警告信息
        Error,    // 错误信息
        Fatal     // 致命错误
    };

    /**
     * @brief 初始化日志系统
     * @param level 设置日志级别
     * @param enableConsole 是否输出到控制台
     * @param enableFile 是否输出到文件
     * @param logDir 日志文件目录，默认为应用程序目录下的logs
     */
    static void init(LogLevel level = Debug, bool enableConsole = true, 
                    bool enableFile = true, const QString& logDir = QString());

    /**
     * @brief 设置当前日志级别
     * @param level 日志级别
     */
    static void setLogLevel(LogLevel level);

    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    static LogLevel getLogLevel();

    /**
     * @brief 设置是否启用控制台输出
     * @param enable 是否启用
     */
    static void setConsoleOutput(bool enable);

    /**
     * @brief 设置是否启用文件输出
     * @param enable 是否启用
     */
    static void setFileOutput(bool enable);

    /**
     * @brief 设置日志文件目录
     * @param dir 日志目录路径
     */
    static void setLogDirectory(const QString& dir);

    /**
     * @brief 记录调试级别日志
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源代码行号
     * @param function 函数名
     */
    static void debug(const QString& message, const char* file = nullptr, 
                     int line = 0, const char* function = nullptr);

    /**
     * @brief 记录信息级别日志
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源代码行号
     * @param function 函数名
     */
    static void info(const QString& message, const char* file = nullptr, 
                    int line = 0, const char* function = nullptr);

    /**
     * @brief 记录警告级别日志
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源代码行号
     * @param function 函数名
     */
    static void warning(const QString& message, const char* file = nullptr, 
                       int line = 0, const char* function = nullptr);

    /**
     * @brief 记录错误级别日志
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源代码行号
     * @param function 函数名
     */
    static void error(const QString& message, const char* file = nullptr, 
                     int line = 0, const char* function = nullptr);

    /**
     * @brief 记录致命错误级别日志
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源代码行号
     * @param function 函数名
     */
    static void fatal(const QString& message, const char* file = nullptr, 
                     int line = 0, const char* function = nullptr);

private:
    /**
     * @brief 内部日志记录函数
     * @param level 日志级别
     * @param message 日志消息
     * @param file 源文件名
     * @param line 源代码行号
     * @param function 函数名
     */
    static void log(LogLevel level, const QString& message, 
                   const char* file, int line, const char* function);

    /**
     * @brief 获取日志级别的字符串表示
     * @param level 日志级别
     * @return 日志级别的字符串
     */
    static QString getLevelString(LogLevel level);

    /**
     * @brief 确保日志文件已打开
     * @return 是否成功打开日志文件
     */
    static bool ensureLogFileOpen();

    // 静态成员变量
    static LogLevel s_logLevel;
    static bool s_consoleEnabled;
    static bool s_fileEnabled;
    static QString s_logDirectory;
    static QFile s_logFile;
    static QTextStream s_logStream;
    static QMutex s_mutex;
    static bool s_initialized;
};

// 日志宏定义
#define LOG_DEBUG(msg) Logger::debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg) Logger::info(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg) Logger::warning(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) Logger::error(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(msg) Logger::fatal(msg, __FILE__, __LINE__, __FUNCTION__)

#endif // LOGGER_H 