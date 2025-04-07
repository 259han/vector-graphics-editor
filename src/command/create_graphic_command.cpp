#include "create_graphic_command.h"
#include "../ui/draw_area.h"
#include "../utils/logger.h"
#include "../core/graphics_item_factory.h"
#include "../core/graphic_item.h"
#include <QGraphicsScene>
#include <QApplication>

CreateGraphicCommand::CreateGraphicCommand(DrawArea* drawArea, 
                                         Graphic::GraphicType type,
                                         const std::vector<QPointF>& points,
                                         const QPen& pen,
                                         const QBrush& brush)
    : m_drawArea(drawArea),
      m_type(type),
      m_points(points),
      m_pen(pen),
      m_brush(brush)
{
    Logger::debug(QString("CreateGraphicCommand: 创建图形命令 - 类型: %1, 点数: %2")
        .arg(static_cast<int>(type))
        .arg(points.size()));
}

CreateGraphicCommand::~CreateGraphicCommand()
{
    // 注意：不删除 m_createdItem，因为它属于场景
    Logger::debug("CreateGraphicCommand: 销毁图形创建命令");
}

void CreateGraphicCommand::execute()
{
    if (m_executed || !m_drawArea) {
        Logger::warning("CreateGraphicCommand::execute: 命令已执行或DrawArea无效");
        return;
    }
    
    Logger::debug("CreateGraphicCommand::execute: 开始执行创建图形命令");
    
    // 确保场景可用
    QGraphicsScene* scene = m_drawArea->scene();
    if (!scene) {
        Logger::error("CreateGraphicCommand::execute: 场景无效");
        return;
    }
    
    // 创建图形项
    if (m_createdItem == nullptr) {
        Logger::debug(QString("CreateGraphicCommand::execute: 创建图形项 - 类型: %1, 点数: %2")
            .arg(static_cast<int>(m_type))
            .arg(m_points.size()));
            
        m_createdItem = m_drawArea->getGraphicFactory()->createCustomItem(m_type, m_points);
        
        if (!m_createdItem) {
            Logger::error("CreateGraphicCommand::execute: 图形工厂创建图形失败");
            return;
        }
        
        // 设置样式
        if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(m_createdItem)) {
            graphicItem->setPen(m_pen);
            graphicItem->setBrush(m_brush);
            Logger::debug("CreateGraphicCommand::execute: 设置图形样式完成");
        }
        
        // 设置图形项标志，确保它可以被选择和移动
        m_createdItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_createdItem->setFlag(QGraphicsItem::ItemIsMovable, true);
        Logger::debug("CreateGraphicCommand::execute: 设置图形标志完成");
    }
    
    // 添加到场景
    if (m_createdItem) {
        Logger::debug(QString("CreateGraphicCommand::execute: 将图形项添加到场景, 指针: %1")
            .arg(reinterpret_cast<quintptr>(m_createdItem)));
        
        scene->addItem(m_createdItem);
        m_executed = true;
        
        // 强制更新场景，但不立即处理事件
        // 这避免了在图形创建过程中可能出现的递归事件处理
        scene->update();
        if (m_drawArea && m_drawArea->viewport()) {
            m_drawArea->viewport()->update();
            // 不在这里调用processEvents，让事件循环自然处理更新
        }
        
        Logger::info(QString("CreateGraphicCommand::execute: 创建图形命令执行成功 - 类型: %1")
            .arg(static_cast<int>(m_type)));
    } else {
        Logger::error("CreateGraphicCommand::execute: 创建图形项为空，无法添加到场景");
    }
}

void CreateGraphicCommand::undo()
{
    if (!m_executed || !m_drawArea || !m_createdItem) {
        return;
    }
    
    // 从场景中移除图形项
    QGraphicsScene* scene = m_drawArea->scene();
    if (scene) {
        scene->removeItem(m_createdItem);
        
        // 强制更新场景和视图
        scene->update();
        if (m_drawArea && m_drawArea->viewport()) {
            m_drawArea->viewport()->update();
            QApplication::processEvents();
        }
    }
    
    m_executed = false;
    
    Logger::info(QString("CreateGraphicCommand: 撤销创建图形命令 - 类型: %1")
        .arg(static_cast<int>(m_type)));
}

QString CreateGraphicCommand::getDescription() const
{
    return QString("创建%1").arg(Graphic::graphicTypeToString(m_type));
}

QString CreateGraphicCommand::getType() const
{
    return "create";
} 