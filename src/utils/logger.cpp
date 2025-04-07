#include "logger.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMutex>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <iostream>

// 静态成员初始化
Logger::LogLevel Logger::s_logLevel = Logger::Debug;
bool Logger::s_consoleEnabled = false;  // 默认关闭控制台输出
bool Logger::s_fileEnabled = true;      // 默认启用文件输出
QString Logger::s_logDirectory;
QFile Logger::s_logFile;
QTextStream Logger::s_logStream;
QMutex Logger::s_mutex;
bool Logger::s_initialized = false;

void Logger::init(LogLevel level, bool enableConsole, bool enableFile, const QString& logDir) {
    QMutexLocker locker(&s_mutex);
    
    s_logLevel = level;
    s_consoleEnabled = enableConsole;
    s_fileEnabled = enableFile;
    
    // 设置日志目录
    if (logDir.isEmpty()) {
        // 获取应用程序可执行文件路径
        QString appPath = QCoreApplication::applicationFilePath();
        QFileInfo appFileInfo(appPath);
        
        // 获取应用程序可执行文件所在目录
        QString appDir = appFileInfo.absolutePath();
        
        // 向上一级找到项目根目录
        QDir rootDir(appDir);
        // 如果在build子目录中，尝试向上走一级
        if (rootDir.dirName().toLower() == "build" || 
            rootDir.dirName().toLower() == "debug" || 
            rootDir.dirName().toLower() == "release") {
            rootDir.cdUp();
        }
        
        // 设置日志目录在项目根目录下的logs子目录
        s_logDirectory = rootDir.absolutePath() + "/logs";
    } else {
        s_logDirectory = logDir;
    }
    
    // 创建日志目录
    if (s_fileEnabled) {
        QDir dir(s_logDirectory);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        
        // 打开日志文件
        ensureLogFileOpen();
    }
    
    s_initialized = true;
}

void Logger::setLogLevel(LogLevel level) {
    QMutexLocker locker(&s_mutex);
    s_logLevel = level;
}

Logger::LogLevel Logger::getLogLevel() {
    QMutexLocker locker(&s_mutex);
    return s_logLevel;
}

void Logger::setConsoleOutput(bool enable) {
    QMutexLocker locker(&s_mutex);
    s_consoleEnabled = enable;
}

void Logger::setFileOutput(bool enable) {
    QMutexLocker locker(&s_mutex);
    s_fileEnabled = enable;
    
    if (enable) {
        ensureLogFileOpen();
    } else {
        if (s_logFile.isOpen()) {
            s_logStream.flush();
            s_logFile.close();
        }
    }
}

void Logger::setLogDirectory(const QString& dir) {
    QMutexLocker locker(&s_mutex);
    s_logDirectory = dir;
    
    // 如果已启用文件输出，则重新打开日志文件
    if (s_fileEnabled) {
        if (s_logFile.isOpen()) {
            s_logStream.flush();
            s_logFile.close();
        }
        ensureLogFileOpen();
    }
}

void Logger::debug(const QString& message, const char* file, int line, const char* function) {
    log(Debug, message, file, line, function);
}

void Logger::info(const QString& message, const char* file, int line, const char* function) {
    log(Info, message, file, line, function);
}

void Logger::warning(const QString& message, const char* file, int line, const char* function) {
    log(Warning, message, file, line, function);
}

void Logger::error(const QString& message, const char* file, int line, const char* function) {
    log(Error, message, file, line, function);
}

void Logger::fatal(const QString& message, const char* file, int line, const char* function) {
    log(Fatal, message, file, line, function);
}

void Logger::log(LogLevel level, const QString& message, 
                const char* file, int line, const char* function) {
    // 如果消息级别低于当前级别，忽略
    if (level < s_logLevel) return;
    
    QMutexLocker locker(&s_mutex);
    
    // 确保已初始化
    if (!s_initialized) {
        init();
    }
    
    // 创建日志条目
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = getLevelString(level);
    
    QString entry;
    
    // 如果提供了文件和行号信息，则包含它们
    if (file && line > 0 && function) {
        // 提取仅文件名，不包含路径
        QString filename(file);
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
            filename = filename.mid(lastSlash + 1);
        } else {
            lastSlash = filename.lastIndexOf('\\');
            if (lastSlash >= 0) {
                filename = filename.mid(lastSlash + 1);
            }
        }
        
        entry = QString("[%1] [%2] [%3:%4 %5] %6")
            .arg(timestamp)
            .arg(levelStr)
            .arg(filename)
            .arg(line)
            .arg(function)
            .arg(message);
    } else {
        entry = QString("[%1] [%2] %3")
            .arg(timestamp)
            .arg(levelStr)
            .arg(message);
    }
    
    // 输出到控制台
    if (s_consoleEnabled) {
        switch (level) {
            case Debug:
                qDebug() << entry;
                break;
            case Info:
                qInfo() << entry;
                break;
            case Warning:
                qWarning() << entry;
                break;
            case Error:
            case Fatal:
                qCritical() << entry;
                break;
        }
    }
    
    // 输出到文件
    if (s_fileEnabled && ensureLogFileOpen()) {
        s_logStream << entry << "\n";
        s_logStream.flush();
    }
    
    // 如果是致命错误，终止应用程序
    if (level == Fatal) {
        abort();
    }
}

QString Logger::getLevelString(LogLevel level) {
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO";
        case Warning: return "WARNING";
        case Error:   return "ERROR";
        case Fatal:   return "FATAL";
        default:      return "UNKNOWN";
    }
}

bool Logger::ensureLogFileOpen() {
    if (s_logFile.isOpen()) {
        return true;
    }
    
    // 创建日志文件名，基于当前日期
    QString logFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    QString logFilePath = s_logDirectory + "/" + logFileName;
    
    s_logFile.setFileName(logFilePath);
    bool success = s_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    
    if (success) {
        s_logStream.setDevice(&s_logFile);
        // 添加日志文件头
        s_logStream << "\n=== Log started at " 
                   << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                   << " ===\n";
        s_logStream.flush();
        
        // 记录日志目录路径，方便查找
        s_logStream << "=== Log directory: " << s_logDirectory << " ===\n";
        s_logStream.flush();
    } else {
        qCritical() << "无法打开日志文件:" << logFilePath;
    }
    
    return success;
} 