#ifndef SELECTION_COMMAND_H
#define SELECTION_COMMAND_H

#include "command/command.h"
#include <QList>
#include <QGraphicsItem>
#include <QPointF>

class DrawArea;

// 选择操作命令 - 用于撤销/重做选择区域内的操作
class SelectionCommand : public Command
{
public:
    // 命令类型枚举
    enum SelectionCommandType {
        MoveSelection,   // 移动选择区域
        DeleteSelection, // 删除选择区域
        ClipSelection    // 裁剪选择区域
    };

    // 构造函数
    SelectionCommand(DrawArea* drawArea, SelectionCommandType type);
    
    // 虚析构函数
    virtual ~SelectionCommand();
    
    // 执行命令 (实现自Command基类)
    virtual void execute() override;
    
    // 撤销命令 (实现自Command基类)
    virtual void undo() override;
    
    // 设置移动选择区域的信息
    void setMoveInfo(const QList<QGraphicsItem*>& items, const QPointF& offset);
    
    // 设置删除选择区域的信息
    void setDeleteInfo(const QList<QGraphicsItem*>& items);
    
    // 设置裁剪选择区域的信息
    void setClipInfo(const QList<QGraphicsItem*>& originalItems, 
                     const QList<QGraphicsItem*>& clippedItems);

private:
    DrawArea* m_drawArea;
    SelectionCommandType m_type;
    
    // 移动命令相关
    QList<QGraphicsItem*> m_items;
    QPointF m_offset;
    
    // 删除命令相关
    struct ItemState {
        QGraphicsItem* item;
        QPointF position;
        bool isSelected;
        // 可以添加其他需要保存的状态
    };
    QList<ItemState> m_itemStates;
    
    // 裁剪命令相关
    QList<QGraphicsItem*> m_originalItems;
    QList<QGraphicsItem*> m_clippedItems;
    
    // 保存图形项的状态
    void saveItemStates();
    
    // 恢复图形项的状态
    void restoreItemStates();
};

#endif // SELECTION_COMMAND_H 