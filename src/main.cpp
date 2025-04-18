#include "ui/main_window.h"
#include "utils/logger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 在创建 QApplication 后立即初始化日志系统
    Logger::init(Logger::Debug, true, true);
    
    Logger::info("应用程序启动");
    
    MainWindow w;
    w.show();
    
    Logger::info("主窗口显示完成");
    
    return a.exec();
} 