#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "command.h"
#include "composite_command.h"
#include <QStack>
#include <QObject>
#include <QReadWriteLock>
#include <QElapsedTimer>
#include <memory>
#include <QList>

class CommandManager : public QObject {
    Q_OBJECT
    
public:
    // 获取单例实例（线程安全，使用C++11的Magic Static特性）
    static CommandManager& getInstance();
    
    // 析构函数必须为public，使unique_ptr能够正确删除
    ~CommandManager();
    
    // 执行命令并将其放入撤销栈
    void executeCommand(Command* command);
    
    // 撤销最近的命令
    void undo();
    
    // 重做最近撤销的命令
    void redo();
    
    // 清除所有命令
    void clear();
    
    // 判断是否可以撤销/重做
    bool canUndo() const;
    bool canRedo() const;
    
    // 获取撤销和重做栈的大小
    int undoStackSize() const;
    int redoStackSize() const;
    
    // 设置最大堆栈大小
    void setMaxStackSize(int size);
    
    // 命令分组控制 - 防止连续命令被合并
    void beginCommandGroup();
    void endCommandGroup();
    bool isGrouping() const { return m_grouping; }
    
    // 添加命令到当前分组
    void addCommandToGroup(Command* command);
    
    // 提交当前命令分组
    void commitCommandGroup();
    
signals:
    void commandExecuted(Command* command);
    void commandUndone(Command* command);
    void commandRedone(Command* command);
    void stackCleared();
    
private:
    CommandManager();
    
    // 禁止复制和赋值
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    
    // 命令堆栈的读写锁
    mutable QReadWriteLock m_lock;
    
    // 命令堆栈
    QStack<Command*> m_undoStack;
    QStack<Command*> m_redoStack;
    
    // 当前命令分组
    QList<Command*> m_currentGroup;
    
    // 最大堆栈大小
    int m_maxStackSize = 100;
    
    // 命令分组状态
    bool m_grouping = false;
    
    // 防抖动控制
    QElapsedTimer m_lastActionTimer;
    const int m_debounceInterval = 100; // 毫秒
    
    // 修剪堆栈大小
    void trimUndoStack();
};

#endif // COMMAND_MANAGER_H 