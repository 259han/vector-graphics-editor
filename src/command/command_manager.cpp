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
    m_lastActionTimer.start();
}

CommandManager::~CommandManager()
{
    clear();
}

void CommandManager::beginCommandGroup()
{
    QWriteLocker locker(&m_lock);
    m_grouping = true;
}

void CommandManager::endCommandGroup()
{
    QWriteLocker locker(&m_lock);
    m_grouping = false;
}

void CommandManager::addCommandToGroup(Command* command)
{
    if (!command) {
        return;
    }

    QWriteLocker locker(&m_lock);
    if (!m_grouping) {
        locker.unlock();
        executeCommand(command);
        return;
    }

    command->execute();
    m_currentGroup.append(command);
    
    locker.unlock();
    emit commandExecuted(command);
}

void CommandManager::commitCommandGroup()
{
    QWriteLocker locker(&m_lock);
    
    if (m_currentGroup.isEmpty()) {
        return;
    }
    
    CompositeCommand* groupCommand = new CompositeCommand(m_currentGroup);
    m_currentGroup.clear();
    
    m_undoStack.push(groupCommand);
    trimUndoStack();
    
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    
    locker.unlock();
    emit commandExecuted(groupCommand);
}

void CommandManager::executeCommand(Command* command)
{
    if (!command) {
        Logger::warning("CommandManager::executeCommand: 传入空命令指针");
        return;
    }
    
    Logger::debug(QString("CommandManager::executeCommand: 执行命令 '%1' [类型: %2]%3")
                 .arg(command->getDescription())
                 .arg(command->getType())
                 .arg(m_grouping ? " (分组中)" : ""));
    
    {
        QWriteLocker locker(&m_lock);
        
        command->execute();
        m_undoStack.push(command);
        trimUndoStack();
        
        qDeleteAll(m_redoStack);
        m_redoStack.clear();
        
        Logger::debug(QString("CommandManager: 撤销栈大小: %1, 重做栈大小: %2")
                     .arg(m_undoStack.size())
                     .arg(m_redoStack.size()));
    }
    
    emit commandExecuted(command);
}

void CommandManager::undo()
{
    if (m_lastActionTimer.elapsed() < m_debounceInterval) {
        Logger::debug("CommandManager::undo: 忽略快速连续撤销请求");
        return;
    }
    
    Command* command = nullptr;
    
    {
        QWriteLocker locker(&m_lock);
        
        if (m_undoStack.isEmpty()) {
            Logger::warning("CommandManager::undo: 撤销栈为空，无法撤销");
            return;
        }
        
        m_lastActionTimer.restart();
        command = m_undoStack.pop();
        
        QString lastCmdDesc = command->getDescription();
        QString lastCmdType = command->getType();
        
        Logger::debug(QString("CommandManager::undo: 撤销命令 '%1' [类型: %2]")
                     .arg(lastCmdDesc)
                     .arg(lastCmdType));
        
        command->undo();
        m_redoStack.push(command);
        
        Logger::debug(QString("CommandManager: 撤销后 - 撤销栈大小: %1, 重做栈大小: %2")
                     .arg(m_undoStack.size())
                     .arg(m_redoStack.size()));
    }
    
    emit commandUndone(command);
}

void CommandManager::redo()
{
    if (m_lastActionTimer.elapsed() < m_debounceInterval) {
        Logger::debug("CommandManager::redo: 忽略快速连续重做请求");
        return;
    }
    
    Command* command = nullptr;
    
    {
        QWriteLocker locker(&m_lock);
        
        if (m_redoStack.isEmpty()) {
            return;
        }
        
        m_lastActionTimer.restart();
        command = m_redoStack.pop();
        command->execute();
        m_undoStack.push(command);
    }
    
    emit commandRedone(command);
}

void CommandManager::clear()
{
    {
        QWriteLocker locker(&m_lock);
        qDeleteAll(m_undoStack);
        m_undoStack.clear();
        qDeleteAll(m_redoStack);
        m_redoStack.clear();
    }
    
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
    while (m_undoStack.size() > m_maxStackSize) {
        delete m_undoStack.takeFirst();
    }
} 