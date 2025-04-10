#include "composite_command.h"
#include "../utils/logger.h"

CompositeCommand::CompositeCommand(const QList<Command*>& commands)
    : m_commands(commands)
{
    Logger::debug(QString("CompositeCommand: 创建新组合命令，包含 %1 个子命令").arg(commands.size()));
}

CompositeCommand::~CompositeCommand()
{
    // 清理所有子命令
    qDeleteAll(m_commands);
    m_commands.clear();
}

void CompositeCommand::execute()
{
    Logger::debug(QString("CompositeCommand::execute: 执行组合命令 (共 %1 个子命令)")
                 .arg(m_commands.size()));
    
    // 执行所有子命令
    for (Command* cmd : m_commands) {
        if (cmd) {
            cmd->execute();
        }
    }
}

void CompositeCommand::undo()
{
    Logger::debug(QString("CompositeCommand::undo: 撤销组合命令 (共 %1 个子命令)")
                 .arg(m_commands.size()));
    
    // 以相反的顺序撤销所有子命令
    for (int i = m_commands.size() - 1; i >= 0; --i) {
        Command* cmd = m_commands.at(i);
        if (cmd) {
            cmd->undo();
        }
    }
}

QString CompositeCommand::getDescription() const
{
    if (m_commands.isEmpty()) {
        return QString("空组合命令");
    }
    
    if (m_commands.size() == 1) {
        return m_commands.first()->getDescription();
    }
    
    // 返回第一个子命令的描述作为组合命令的描述，并添加子命令数量信息
    return QString("%1 (组合: %2 个操作)")
            .arg(m_commands.first()->getDescription())
            .arg(m_commands.size());
}

QString CompositeCommand::getType() const
{
    if (m_commands.isEmpty()) {
        return QString("CompositeCommand");
    }
    
    // 返回第一个子命令的类型作为组合命令的类型
    return QString("CompositeCommand:%1").arg(m_commands.first()->getType());
}