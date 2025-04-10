#ifndef COMPOSITE_COMMAND_H
#define COMPOSITE_COMMAND_H

#include "command.h"
#include <QList>

/**
 * @brief 组合命令类，用于将多个命令作为一个单独的命令执行
 * 
 * 遵循命令模式的组合模式实现，允许将一组命令组合成一个单独的命令对象
 * 用于支持命令分组、批量撤销和重做功能
 */
class CompositeCommand : public Command {
public:
    /**
     * @brief 构造一个组合命令
     * @param commands 要组合的命令列表
     */
    CompositeCommand(const QList<Command*>& commands);
    
    /**
     * @brief 析构函数
     * 
     * 释放所有子命令的内存
     */
    virtual ~CompositeCommand();
    
    /**
     * @brief 执行所有子命令
     */
    void execute() override;
    
    /**
     * @brief 以相反的顺序撤销所有子命令
     */
    void undo() override;
    
    /**
     * @brief 获取组合命令的描述
     * @return 组合命令的描述字符串
     */
    QString getDescription() const override;
    
    /**
     * @brief 获取组合命令的类型
     * @return 组合命令的类型字符串
     */
    QString getType() const override;
    
private:
    // 子命令列表
    QList<Command*> m_commands;
};

#endif // COMPOSITE_COMMAND_H 