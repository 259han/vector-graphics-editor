#include "command/command_manager.h"
#include <QDebug>

CommandManager* CommandManager::m_instance = nullptr;

CommandManager* CommandManager::getInstance()
{
    if (!m_instance) {
        m_instance = new CommandManager();
    }
    return m_instance;
}

CommandManager::CommandManager()
{
    qDebug() << "Command Manager: Created";
}

CommandManager::~CommandManager()
{
    clear();
}

void CommandManager::executeCommand(Command* command)
{
    if (!command) {
        return;
    }
    
    // 执行命令
    command->execute();
    
    // 添加到撤销栈
    m_undoStack.push(command);
    
    // 清除重做栈
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    
    // 发送信号
    emit commandExecuted(command);
    
    qDebug() << "Command Manager: Command executed, undo stack size:" << m_undoStack.size();
}

void CommandManager::undo()
{
    if (m_undoStack.isEmpty()) {
        qDebug() << "Command Manager: Cannot undo, stack is empty";
        return;
    }
    
    // 获取最近的命令
    Command* command = m_undoStack.pop();
    
    // 撤销命令
    command->undo();
    
    // 添加到重做栈
    m_redoStack.push(command);
    
    // 发送信号
    emit commandUndone(command);
    
    qDebug() << "Command Manager: Command undone, redo stack size:" << m_redoStack.size();
}

void CommandManager::redo()
{
    if (m_redoStack.isEmpty()) {
        qDebug() << "Command Manager: Cannot redo, stack is empty";
        return;
    }
    
    // 获取最近撤销的命令
    Command* command = m_redoStack.pop();
    
    // 重新执行命令
    command->execute();
    
    // 添加回撤销栈
    m_undoStack.push(command);
    
    // 发送信号
    emit commandRedone(command);
    
    qDebug() << "Command Manager: Command redone, undo stack size:" << m_undoStack.size();
}

void CommandManager::clear()
{
    // 清除撤销栈
    qDeleteAll(m_undoStack);
    m_undoStack.clear();
    
    // 清除重做栈
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    
    // 发送信号
    emit stackCleared();
    
    qDebug() << "Command Manager: Command stacks cleared";
}

bool CommandManager::canUndo() const
{
    return !m_undoStack.isEmpty();
}

bool CommandManager::canRedo() const
{
    return !m_redoStack.isEmpty();
} 