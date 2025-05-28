#include "command/selection_command.h"
#include "ui/draw_area.h"
#include "../utils/logger.h"
#include "../core/flowchart_base_item.h"
#include <QGraphicsScene>
#include <QException>

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
            
            // 从场景中移除所有选中的图形项，并从ConnectionManager注销流程图元素
            for (QGraphicsItem* item : m_items) {
                if (item->scene()) {
                    // 如果是流程图元素，先从ConnectionManager注销
                    FlowchartBaseItem* flowchartItem = dynamic_cast<FlowchartBaseItem*>(item);
                    if (flowchartItem && m_drawArea && m_drawArea->getConnectionManager()) {
                        m_drawArea->getConnectionManager()->unregisterFlowchartItem(flowchartItem);
                        Logger::debug(QString("从ConnectionManager注销流程图元素: %1").arg(flowchartItem->getGraphicType()));
                    }
                    
                    item->scene()->removeItem(item);
                }
            }
            break;
            
    }
}

void SelectionCommand::undo()
{
    if (!m_drawArea) {
        Logger::warning("SelectionCommand::undo: DrawArea无效");
        return;
    }
    
    QGraphicsScene* scene = m_drawArea->scene();
    if (!scene) {
        Logger::error("SelectionCommand::undo: 场景无效");
        return;
    }
    
    try {
        switch (m_type) {
            case MoveSelection:
                // 将所有选中的图形项移回原位置
                for (QGraphicsItem* item : m_items) {
                    if (item && item->scene() == scene) {
                        item->moveBy(-m_offset.x(), -m_offset.y());
                    } else if (item) {
                        Logger::warning("SelectionCommand::undo: 项目不在当前场景中，无法移动");
                    }
                }
                break;
                
            case DeleteSelection:
                // 恢复删除的图形项
                restoreItemStates();
                break;
        }
        
        // 更新场景
        scene->update();
        if (m_drawArea->viewport()) {
            m_drawArea->viewport()->update();
        }
    } catch (const std::exception& e) {
        Logger::error(QString("SelectionCommand::undo: 异常 - %1").arg(e.what()));
    } catch (...) {
        Logger::error("SelectionCommand::undo: 未知异常");
    }
}

QString SelectionCommand::getDescription() const
{
    int itemCount = m_items.size();
    
    switch (m_type) {
        case MoveSelection:
            return QString("移动%1个图形项 (%2, %3)")
                    .arg(itemCount)
                    .arg(m_offset.x())
                    .arg(m_offset.y());
            
        case DeleteSelection:
            return QString("删除%1个图形项").arg(itemCount);
            
        default:
            return QString("选择操作");
    }
}

QString SelectionCommand::getType() const
{
    switch (m_type) {
        case MoveSelection:
            return QString("transform");
            
        case DeleteSelection:
            return QString("delete");
            
        default:
            return QString("selection");
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
    if (!m_drawArea || !m_drawArea->scene()) {
        Logger::warning("SelectionCommand::restoreItemStates: DrawArea或场景无效");
        return;
    }
    
    QGraphicsScene* scene = m_drawArea->scene();
    
    for (const ItemState& state : m_itemStates) {
        // 检查项目是否有效
        if (!state.item) {
            Logger::warning("SelectionCommand::restoreItemStates: 项目无效");
            continue;
        }
        
        try {
            // 如果项目不在场景中，则添加它
            if (!state.item->scene()) {
                scene->addItem(state.item);
                Logger::debug(QString("SelectionCommand::restoreItemStates: 将项目 %1 添加回场景")
                              .arg(reinterpret_cast<quintptr>(state.item)));
            }
            
            // 设置位置和选择状态
            state.item->setPos(state.position);
            state.item->setSelected(state.isSelected);
            
            // 如果是流程图元素，重新注册到ConnectionManager
            FlowchartBaseItem* flowchartItem = dynamic_cast<FlowchartBaseItem*>(state.item);
            if (flowchartItem && m_drawArea && m_drawArea->getConnectionManager()) {
                m_drawArea->getConnectionManager()->registerFlowchartItem(flowchartItem);
                Logger::debug(QString("重新注册流程图元素到ConnectionManager: %1").arg(flowchartItem->getGraphicType()));
            }
        } catch (const std::exception& e) {
            Logger::error(QString("SelectionCommand::restoreItemStates: 恢复项目状态时出错 - %1").arg(e.what()));
        } catch (...) {
            Logger::error("SelectionCommand::restoreItemStates: 恢复项目状态时出现未知错误");
        }
    }
} 