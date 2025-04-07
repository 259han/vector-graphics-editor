#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "command.h"
#include <QStack>
#include <QObject>

class CommandManager : public QObject {
    Q_OBJECT
    
public:
    static CommandManager* getInstance();
    
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
    
signals:
    void commandExecuted(Command* command);
    void commandUndone(Command* command);
    void commandRedone(Command* command);
    void stackCleared();
    
private:
    CommandManager();
    ~CommandManager();
    
    static CommandManager* m_instance;
    
    QStack<Command*> m_undoStack;
    QStack<Command*> m_redoStack;
};

#endif // COMMAND_MANAGER_H 