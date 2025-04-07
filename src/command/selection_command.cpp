#include "command/selection_command.h"
#include "ui/draw_area.h"
#include <QGraphicsScene>

SelectionCommand::SelectionCommand(DrawArea* drawArea, SelectionCommandType type)
    : m_drawArea(drawArea)
    , m_type(type)
    , m_offset(0, 0)
{
}

SelectionCommand::~SelectionCommand()
{
    // 释放资源
}

void SelectionCommand::execute()
{
    if (!m_drawArea) {
        return;
    }
    
    switch (m_type) {
        case MoveSelection:
            // 移动所有选中的图形项
            for (QGraphicsItem* item : m_items) {
                item->moveBy(m_offset.x(), m_offset.y());
            }
            break;
            
        case DeleteSelection:
            // 保存当前项的状态以备撤销
            saveItemStates();
            
            // 从场景中移除所有选中的图形项
            for (QGraphicsItem* item : m_items) {
                if (item->scene()) {
                    item->scene()->removeItem(item);
                }
            }
            break;
            
        case ClipSelection:
            // 保存原始图形项的状态
            m_originalItems = m_items;
            
            // 从场景中移除原始图形项
            for (QGraphicsItem* item : m_originalItems) {
                if (item->scene()) {
                    item->scene()->removeItem(item);
                }
            }
            
            // 将裁剪后的图形项添加到场景
            for (QGraphicsItem* item : m_clippedItems) {
                if (!item->scene() && m_drawArea->scene()) {
                    m_drawArea->scene()->addItem(item);
                }
            }
            break;
    }
}

void SelectionCommand::undo()
{
    if (!m_drawArea) {
        return;
    }
    
    switch (m_type) {
        case MoveSelection:
            // 将所有选中的图形项移回原位置
            for (QGraphicsItem* item : m_items) {
                item->moveBy(-m_offset.x(), -m_offset.y());
            }
            break;
            
        case DeleteSelection:
            // 恢复删除的图形项
            restoreItemStates();
            break;
            
        case ClipSelection:
            // 从场景中移除裁剪后的图形项
            for (QGraphicsItem* item : m_clippedItems) {
                if (item->scene()) {
                    item->scene()->removeItem(item);
                }
            }
            
            // 将原始图形项添加回场景
            for (QGraphicsItem* item : m_originalItems) {
                if (!item->scene() && m_drawArea->scene()) {
                    m_drawArea->scene()->addItem(item);
                }
            }
            break;
    }
}

void SelectionCommand::setMoveInfo(const QList<QGraphicsItem*>& items, const QPointF& offset)
{
    m_items = items;
    m_offset = offset;
}

void SelectionCommand::setDeleteInfo(const QList<QGraphicsItem*>& items)
{
    m_items = items;
}

void SelectionCommand::setClipInfo(const QList<QGraphicsItem*>& originalItems, 
                                 const QList<QGraphicsItem*>& clippedItems)
{
    m_originalItems = originalItems;
    m_clippedItems = clippedItems;
}

void SelectionCommand::saveItemStates()
{
    m_itemStates.clear();
    
    for (QGraphicsItem* item : m_items) {
        ItemState state;
        state.item = item;
        state.position = item->pos();
        state.isSelected = item->isSelected();
        
        m_itemStates.append(state);
    }
}

void SelectionCommand::restoreItemStates()
{
    for (const ItemState& state : m_itemStates) {
        if (!state.item->scene() && m_drawArea->scene()) {
            m_drawArea->scene()->addItem(state.item);
        }
        
        state.item->setPos(state.position);
        state.item->setSelected(state.isSelected);
    }
} 