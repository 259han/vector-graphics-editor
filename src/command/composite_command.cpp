#include "composite_command.h"
#include "../utils/logger.h"

CompositeCommand::CompositeCommand(const QList<Command*>& commands)
    : m_commands(commands)
{
    Logger::debug(QString("CompositeCommand: 创建新组合命令，包含 %1 个子命令").arg(commands.size()));
}

CompositeCommand::~CompositeCommand()
{
    qDeleteAll(m_commands);
    m_commands.clear();
}

void CompositeCommand::execute()
{
    Logger::debug(QString("CompositeCommand::execute: 执行组合命令 (共 %1 个子命令)")
                 .arg(m_commands.size()));
    
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
    
    return QString("%1 (组合: %2 个操作)")
            .arg(m_commands.first()->getDescription())
            .arg(m_commands.size());
}

QString CompositeCommand::getType() const
{
    if (m_commands.isEmpty()) {
        return QString("CompositeCommand");
    }
    
    return QString("CompositeCommand:%1").arg(m_commands.first()->getType());
}