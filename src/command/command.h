#ifndef COMMAND_H
#define COMMAND_H

#include <QString>
#include <QDataStream>

class GraphicManager;

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    
    // 获取命令描述，用于显示在历史面板
    virtual QString getDescription() const = 0;
    
    // 获取命令类型，用于图标显示或分组
    virtual QString getType() const = 0;
};

#endif // COMMAND_H