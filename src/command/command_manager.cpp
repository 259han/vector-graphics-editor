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
    m_lastActionTimer.start(); // 初始化计时器
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
        // 如果不在分组模式，单独执行此命令
        locker.unlock();
        executeCommand(command);
        return;
    }

    // 执行命令并添加到当前分组中
    command->execute();
    m_currentGroup.append(command);
    
    // 在锁外发送信号
    locker.unlock();
    emit commandExecuted(command);
}

void CommandManager::commitCommandGroup()
{
    QWriteLocker locker(&m_lock);
    
    if (m_currentGroup.isEmpty()) {
        return;
    }
    
    // 创建组合命令
    CompositeCommand* groupCommand = new CompositeCommand(m_currentGroup);
    m_currentGroup.clear();
    
    // 添加到撤销栈
    m_undoStack.push(groupCommand);
    
    // 检查是否需要修剪撤销栈大小
    trimUndoStack();
    
    // 清除重做栈
    qDeleteAll(m_redoStack);
    m_redoStack.clear();
    
    // 发送信号
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
        
        // 执行命令
        command->execute();
        
        // 添加到撤销栈
        m_undoStack.push(command);
        
        // 检查是否需要修剪撤销栈大小
        trimUndoStack();
        
        // 清除重做栈
        qDeleteAll(m_redoStack);
        m_redoStack.clear();
        
        // 调试: 输出堆栈信息
        Logger::debug(QString("CommandManager: 撤销栈大小: %1, 重做栈大小: %2")
                     .arg(m_undoStack.size())
                     .arg(m_redoStack.size()));
    }
    
    // 在锁外发送信号
    emit commandExecuted(command);
}

void CommandManager::undo()
{
    // 防抖动检查：如果与上次操作时间间隔太短，则忽略
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
        
        // 重置计时器
        m_lastActionTimer.restart();
        
        // 获取最近的命令
        command = m_undoStack.pop();
        
        // 记录这个命令的描述和类型，用于检测是否多次执行相同命令
        QString lastCmdDesc = command->getDescription();
        QString lastCmdType = command->getType();
        
        Logger::debug(QString("CommandManager::undo: 撤销命令 '%1' [类型: %2]")
                     .arg(lastCmdDesc)
                     .arg(lastCmdType));
        
        // 撤销命令
        command->undo();
        
        // 添加到重做栈
        m_redoStack.push(command);
        
        // 调试: 输出堆栈信息
        Logger::debug(QString("CommandManager: 撤销后 - 撤销栈大小: %1, 重做栈大小: %2")
                     .arg(m_undoStack.size())
                     .arg(m_redoStack.size()));
    }
    
    // 在锁外发送信号
    emit commandUndone(command);
}

void CommandManager::redo()
{
    // 防抖动检查：如果与上次操作时间间隔太短，则忽略
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
        
        // 重置计时器
        m_lastActionTimer.restart();
        
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
        Command* oldestCommand = m_undoStack.takeFirst();
        delete oldestCommand;
    }
} 