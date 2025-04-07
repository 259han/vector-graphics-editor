#include "command/command_manager.h"
#include "../utils/logger.h"
#include <QDebug>
#include <QReadLocker>
#include <QWriteLocker>
#include <QApplication>

// 删除静态成员初始化
// std::unique_ptr<CommandManager> CommandManager::m_instance;
// QMutex CommandManager::m_mutex;

CommandManager& CommandManager::getInstance()
{
    static CommandManager instance;
    return instance;
}

CommandManager::CommandManager()
{
    Logger::info("CommandManager: 初始化");
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
    
    {
        QWriteLocker locker(&m_lock);
        
        // 执行命令
        command->execute();
        
        // 添加到撤销栈
        m_undoStack.push(command);
        
        // 检查是否需要修剪撤销栈大小
        trimUndoStack();
        
        // 清除重做栈
        qDeleteAll(m_redoStack);
        m_redoStack.clear();
    }
    
    // 在锁外发送信号
    emit commandExecuted(command);
}

void CommandManager::undo()
{
    Command* command = nullptr;
    
    {
        QWriteLocker locker(&m_lock);
        
        if (m_undoStack.isEmpty()) {
            return;
        }
        
        // 获取最近的命令
        command = m_undoStack.pop();
        
        // 撤销命令
        command->undo();
        
        // 添加到重做栈
        m_redoStack.push(command);
    }
    
    // 在锁外发送信号
    emit commandUndone(command);
}

void CommandManager::redo()
{
    Command* command = nullptr;
    
    {
        QWriteLocker locker(&m_lock);
        
        if (m_redoStack.isEmpty()) {
            return;
        }
        
        // 获取最近撤销的命令
        command = m_redoStack.pop();
        
        // 重新执行命令
        command->execute();
        
        // 添加回撤销栈
        m_undoStack.push(command);
    }
    
    // 在锁外发送信号
    emit commandRedone(command);
}

void CommandManager::clear()
{
    {
        QWriteLocker locker(&m_lock);
        
        // 清除撤销栈
        qDeleteAll(m_undoStack);
        m_undoStack.clear();
        
        // 清除重做栈
        qDeleteAll(m_redoStack);
        m_redoStack.clear();
    }
    
    // 在锁外发送信号
    emit stackCleared();
}

bool CommandManager::canUndo() const
{
    QReadLocker locker(&m_lock);
    return !m_undoStack.isEmpty();
}

bool CommandManager::canRedo() const
{
    QReadLocker locker(&m_lock);
    return !m_redoStack.isEmpty();
}

int CommandManager::undoStackSize() const
{
    QReadLocker locker(&m_lock);
    return m_undoStack.size();
}

int CommandManager::redoStackSize() const
{
    QReadLocker locker(&m_lock);
    return m_redoStack.size();
}

void CommandManager::setMaxStackSize(int size)
{
    QWriteLocker locker(&m_lock);
    m_maxStackSize = size;
    trimUndoStack();
}

void CommandManager::trimUndoStack()
{
    // 确保不超过最大堆栈大小
    while (m_undoStack.size() > m_maxStackSize) {
        Command* cmd = m_undoStack.takeFirst();
        delete cmd;
    }
} 