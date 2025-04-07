#include "ui/main_window.h"
#include "utils/logger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 在创建 QApplication 后立即初始化日志系统
    // 启用文件输出和控制台输出
    Logger::init(Logger::Debug, true, true);
    
    // 记录应用程序启动
    Logger::info("应用程序启动");
    
    MainWindow w;
    w.show();
    
    // 记录主窗口显示完成
    Logger::info("主窗口显示完成");
    
    return a.exec();
} 