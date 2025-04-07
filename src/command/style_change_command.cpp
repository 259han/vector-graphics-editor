#include "style_change_command.h"
#include "../ui/draw_area.h"
#include "../core/graphic_item.h"
#include "../utils/logger.h"
#include <QGraphicsScene>
#include <QApplication>

StyleChangeCommand::StyleChangeCommand(DrawArea* drawArea, 
                                     const QList<QGraphicsItem*>& items,
                                     StylePropertyType propertyType)
    : m_drawArea(drawArea),
      m_propertyType(propertyType)
{
    // 保存当前样式状态
    saveItemStyles(items);
    
    Logger::debug(QString("StyleChangeCommand: 创建样式变更命令 - 属性类型: %1, 图形项数: %2")
        .arg(static_cast<int>(propertyType))
        .arg(items.size()));
}

StyleChangeCommand::~StyleChangeCommand()
{
    Logger::debug("StyleChangeCommand: 销毁样式变更命令");
}

void StyleChangeCommand::execute()
{
    if (m_itemStates.isEmpty()) {
        Logger::warning("StyleChangeCommand::execute: 没有可应用样式的图形项，执行取消");
        return;
    }
    
    if (m_executed) {
        Logger::warning("StyleChangeCommand::execute: 命令已执行过，不重复执行");
        return;
    }
    
    Logger::debug(QString("StyleChangeCommand::execute: 开始执行样式变更 - 图形项数: %1, 属性类型: %2")
        .arg(m_itemStates.size())
        .arg(static_cast<int>(m_propertyType)));
    
    int successCount = 0;
    
    // 首先收集所有要更新的项，以便一次性重绘
    QList<QGraphicsItem*> updatedItems;
    
    for (const auto& state : m_itemStates) {
        if (!state.item) {
            Logger::warning("StyleChangeCommand::execute: 发现无效的图形项");
            continue;
        }
        
        QPen oldPen = state.item->getPen();
        QBrush oldBrush = state.item->getBrush();
        
        switch (m_propertyType) {
            case PenStyle: {
                state.item->setPen(m_newPen);
                Logger::debug(QString("StyleChangeCommand: 应用画笔样式 - 颜色: %1, 宽度: %2")
                    .arg(m_newPen.color().name())
                    .arg(m_newPen.widthF()));
                break;
            }
            case PenWidth: {
                QPen pen = state.item->getPen();
                pen.setWidthF(m_newPenWidth);
                state.item->setPen(pen);
                Logger::debug(QString("StyleChangeCommand: 应用画笔宽度 - 宽度: %1").arg(m_newPenWidth));
                break;
            }
            case PenColor: {
                QPen pen = state.item->getPen();
                pen.setColor(m_newPenColor);
                state.item->setPen(pen);
                Logger::debug(QString("StyleChangeCommand: 应用画笔颜色 - 颜色: %1").arg(m_newPenColor.name()));
                break;
            }
            case BrushColor: {
                QBrush brush = state.item->getBrush();
                brush.setColor(m_newBrushColor);
                state.item->setBrush(brush);
                Logger::debug(QString("StyleChangeCommand: 应用画刷颜色 - 颜色: %1").arg(m_newBrushColor.name()));
                break;
            }
            case BrushStyle: {
                state.item->setBrush(m_newBrush);
                Logger::debug(QString("StyleChangeCommand: 应用画刷样式 - 颜色: %1").arg(m_newBrush.color().name()));
                break;
            }
        }
        
        // 输出调试信息确认颜色变化
        QPen newPen = state.item->getPen();
        QBrush newBrush = state.item->getBrush();
        Logger::debug(QString("StyleChangeCommand: 变更前后对比 - 画笔颜色: %1 -> %2, 宽度: %3 -> %4")
                .arg(oldPen.color().name())
                .arg(newPen.color().name())
                .arg(oldPen.widthF())
                .arg(newPen.widthF()));
        
        // 更新图形项
        QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(state.item);
        if (item) {
            // 强制更新图形项
            item->update(item->boundingRect());
            updatedItems.append(item);
            successCount++;
        }
    }
    
    // 确保命令执行成功
    if (successCount > 0) {
        if (m_drawArea && m_drawArea->scene()) {
            // 更新整个场景以确保视觉效果变化
            m_drawArea->scene()->update();
            
            // 强制处理更新事件
            QApplication::processEvents();
            
            Logger::debug("StyleChangeCommand: 更新场景完成");
        }
        
        m_executed = true;
        Logger::info(QString("StyleChangeCommand: 成功执行样式变更命令 - 属性类型: %1, 成功项数: %2")
            .arg(static_cast<int>(m_propertyType))
            .arg(successCount));
    } else {
        Logger::warning("StyleChangeCommand: 样式变更命令未成功应用到任何图形项");
    }
}

void StyleChangeCommand::undo()
{
    if (!m_executed) {
        return;
    }
    
    int successCount = 0;
    
    for (const auto& state : m_itemStates) {
        if (!state.item) continue;
        
        // 恢复原始样式
        switch (m_propertyType) {
            case PenStyle:
            case PenWidth:
            case PenColor:
                state.item->setPen(state.oldPen);
                Logger::debug(QString("StyleChangeCommand::undo: 恢复画笔样式 - 颜色: %1, 宽度: %2")
                    .arg(state.oldPen.color().name())
                    .arg(state.oldPen.widthF()));
                break;
            case BrushColor:
            case BrushStyle:
                state.item->setBrush(state.oldBrush);
                Logger::debug(QString("StyleChangeCommand::undo: 恢复画刷样式 - 颜色: %1")
                    .arg(state.oldBrush.color().name()));
                break;
        }
        
        // 更新图形项
        if (QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(state.item)) {
            // 强制更新图形项
            item->update(item->boundingRect());
            successCount++;
        }
    }
    
    // 更新整个场景以确保视觉效果变化
    if (m_drawArea && m_drawArea->scene()) {
        m_drawArea->scene()->update();
        
        // 强制处理更新事件
        QApplication::processEvents();
        
        Logger::debug("StyleChangeCommand::undo: 更新场景完成");
    }
    
    m_executed = false;
    Logger::info(QString("StyleChangeCommand: 撤销样式变更命令 - 属性类型: %1, 成功项数: %2")
        .arg(static_cast<int>(m_propertyType))
        .arg(successCount));
}

void StyleChangeCommand::setNewPen(const QPen& pen)
{
    m_newPen = pen;
}

void StyleChangeCommand::setNewBrush(const QBrush& brush)
{
    m_newBrush = brush;
}

void StyleChangeCommand::setNewPenWidth(qreal width)
{
    m_newPenWidth = width;
}

void StyleChangeCommand::setNewPenColor(const QColor& color)
{
    m_newPenColor = color;
}

void StyleChangeCommand::setNewBrushColor(const QColor& color)
{
    m_newBrushColor = color;
}

void StyleChangeCommand::saveItemStyles(const QList<QGraphicsItem*>& items)
{
    m_itemStates.clear();
    
    Logger::debug(QString("StyleChangeCommand::saveItemStyles: 处理 %1 个图形项").arg(items.size()));
    
    for (QGraphicsItem* item : items) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (graphicItem) {
            ItemState state;
            state.item = graphicItem;
            state.oldPen = graphicItem->getPen();
            state.oldBrush = graphicItem->getBrush();
            m_itemStates.append(state);
            
            Logger::debug(QString("StyleChangeCommand: 保存图形项样式 - 画笔颜色: %1, 画刷颜色: %2")
                .arg(state.oldPen.color().name())
                .arg(state.oldBrush.color().name()));
                
            // 检查当前值与要设置的值是否相同
            if (m_propertyType == PenColor && state.oldPen.color() == m_newPenColor) {
                Logger::warning("StyleChangeCommand: 新旧画笔颜色相同，不会有视觉变化");
            }
            if (m_propertyType == BrushColor && state.oldBrush.color() == m_newBrushColor) {
                Logger::warning("StyleChangeCommand: 新旧画刷颜色相同，不会有视觉变化");
            }
            if (m_propertyType == PenWidth && qFuzzyCompare(state.oldPen.widthF(), m_newPenWidth)) {
                Logger::warning("StyleChangeCommand: 新旧线宽相同，不会有视觉变化");
            }
        } else {
            Logger::warning(QString("StyleChangeCommand: 无法转换为GraphicItem，项目类型: %1")
                .arg(item->type()));
        }
    }
    
    Logger::debug(QString("StyleChangeCommand::saveItemStyles: 成功保存 %1 个图形项样式").arg(m_itemStates.size()));
    
    // 如果是颜色变更，确保新颜色与旧颜色不同
    if (m_propertyType == PenColor && !m_itemStates.isEmpty()) {
        // 如果所有项的颜色都相同，且与新颜色一致，则生成一个不同的颜色
        bool allSameColor = true;
        QColor firstColor = m_itemStates.first().oldPen.color();
        
        for (const auto& state : m_itemStates) {
            if (state.oldPen.color() != firstColor) {
                allSameColor = false;
                break;
            }
        }
        
        if (allSameColor && firstColor == m_newPenColor) {
            // 如果要设置的颜色与当前颜色相同，改为使用红色
            m_newPenColor = (firstColor == Qt::red) ? Qt::blue : Qt::red;
            Logger::warning(QString("StyleChangeCommand: 新旧画笔颜色相同，已调整为 %1 以便看到效果")
                .arg(m_newPenColor.name()));
        }
    }
    
    // 如果是画刷颜色变更，确保新颜色与旧颜色不同
    if (m_propertyType == BrushColor && !m_itemStates.isEmpty()) {
        // 如果所有项的颜色都相同，且与新颜色一致，则生成一个不同的颜色
        bool allSameColor = true;
        QColor firstColor = m_itemStates.first().oldBrush.color();
        
        for (const auto& state : m_itemStates) {
            if (state.oldBrush.color() != firstColor) {
                allSameColor = false;
                break;
            }
        }
        
        if (allSameColor && firstColor == m_newBrushColor) {
            // 如果要设置的颜色与当前颜色相同，改为使用绿色
            m_newBrushColor = (firstColor == Qt::green) ? Qt::yellow : Qt::green;
            Logger::warning(QString("StyleChangeCommand: 新旧画刷颜色相同，已调整为 %1 以便看到效果")
                .arg(m_newBrushColor.name()));
        }
    }
}

QString StyleChangeCommand::getDescription() const
{
    QString items = QString::number(m_itemStates.size());
    QString propertyDesc;
    
    switch (m_propertyType) {
        case PenStyle:
            propertyDesc = "画笔样式";
            break;
        case PenWidth:
            propertyDesc = QString("画笔宽度为%1").arg(m_newPenWidth);
            break;
        case PenColor:
            propertyDesc = "画笔颜色";
            break;
        case BrushColor:
            propertyDesc = "填充颜色";
            break;
        case BrushStyle:
            propertyDesc = "填充样式";
            break;
        default:
            propertyDesc = "样式";
    }
    
    return QString("修改%1个图形的%2").arg(items).arg(propertyDesc);
}

QString StyleChangeCommand::getType() const
{
    return "style";
} 