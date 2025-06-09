#include "create_graphic_command.h"
#include "../ui/draw_area.h"
#include "../utils/logger.h"
#include "../core/graphics_item_factory.h"
#include "../core/graphic_item.h"
#include <QGraphicsScene>
#include <QApplication>
#include <QTimer>

CreateGraphicCommand::CreateGraphicCommand(DrawArea* drawArea, 
                                         GraphicItem::GraphicType type,
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

CreateGraphicCommand::CreateGraphicCommand(QGraphicsScene* scene, GraphicItem* graphicItem)
    : m_scene(scene),
      m_type(graphicItem->getGraphicType()),
      m_pen(graphicItem->getPen()),
      m_brush(graphicItem->getBrush()),
      m_createdItem(graphicItem),
      m_directCreation(true)
{
    m_points = graphicItem->getClipboardPoints();
    Logger::debug(QString("CreateGraphicCommand: 创建直接图形命令 - 类型: %1")
        .arg(static_cast<int>(m_type)));
}

CreateGraphicCommand::~CreateGraphicCommand()
{
    Logger::debug("CreateGraphicCommand: 销毁图形创建命令");
}

void CreateGraphicCommand::execute()
{
    if (m_directCreation) {
        if (m_executed || !m_scene || !m_createdItem) {
            Logger::warning("CreateGraphicCommand::execute: 直接创建模式 - 命令已执行或场景/图形项无效");
            return;
        }
        
        Logger::debug("CreateGraphicCommand::execute: 开始执行直接创建图形命令");
        
        Logger::debug(QString("CreateGraphicCommand::execute: 将图形项添加到场景, 指针: %1")
            .arg(reinterpret_cast<quintptr>(m_createdItem)));
        
        m_scene->addItem(m_createdItem);
        m_executed = true;
        
        if (m_scene) {
            m_scene->update();
        }
        
        Logger::info(QString("CreateGraphicCommand::execute: 直接创建图形命令执行成功 - 类型: %1")
            .arg(static_cast<int>(m_type)));
        return;
    }
    
    if (m_executed || !m_drawArea) {
        Logger::warning("CreateGraphicCommand::execute: 命令已执行或DrawArea无效");
        return;
    }
    
    Logger::debug("CreateGraphicCommand::execute: 开始执行创建图形命令");
    
    QGraphicsScene* scene = m_drawArea->scene();
    if (!scene) {
        Logger::error("CreateGraphicCommand::execute: 场景无效");
        return;
    }
    
    if (m_createdItem == nullptr) {
        Logger::debug(QString("CreateGraphicCommand::execute: 创建图形项 - 类型: %1, 点数: %2")
            .arg(static_cast<int>(m_type))
            .arg(m_points.size()));
            
        m_createdItem = m_drawArea->getGraphicFactory()->createCustomItem(m_type, m_points);
        
        if (!m_createdItem) {
            Logger::error("CreateGraphicCommand::execute: 图形工厂创建图形失败");
            return;
        }
        
        if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(m_createdItem)) {
            graphicItem->setPen(m_pen);
            graphicItem->setBrush(m_brush);
            Logger::debug("CreateGraphicCommand::execute: 设置图形样式完成");
        }
        
        m_createdItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_createdItem->setFlag(QGraphicsItem::ItemIsMovable, true);
        Logger::debug("CreateGraphicCommand::execute: 设置图形标志完成");
    }
    
    if (m_createdItem) {
        Logger::debug(QString("CreateGraphicCommand::execute: 将图形项添加到场景, 指针: %1")
            .arg(reinterpret_cast<quintptr>(m_createdItem)));
        
        scene->addItem(m_createdItem);
        m_executed = true;
        
        if (m_drawArea && m_createdItem) {
            m_drawArea->handleNewGraphicItem(m_createdItem);
        }
        
        scene->update();
        if (m_drawArea && m_drawArea->viewport()) {
            m_drawArea->viewport()->update();
        }
        
        Logger::info(QString("CreateGraphicCommand::execute: 创建图形命令执行成功 - 类型: %1")
            .arg(static_cast<int>(m_type)));
    } else {
        Logger::error("CreateGraphicCommand::execute: 创建图形项为空，无法添加到场景");
    }
}

void CreateGraphicCommand::undo()
{
    if (!m_executed || !m_createdItem) {
        Logger::debug("CreateGraphicCommand::undo: 命令未执行或图形项为空");
        return;
    }
    
    QGraphicsScene* scene = m_directCreation ? m_scene : m_drawArea->scene();
    if (!scene) {
        Logger::warning("CreateGraphicCommand::undo: 场景无效");
        return;
    }
    
    if (!m_createdItem->scene() || m_createdItem->scene() != scene) {
        Logger::warning("CreateGraphicCommand::undo: 图形项不在当前场景中，可能已被删除");
        m_executed = false;
        return;
    }
    
    try {
        scene->removeItem(m_createdItem);
        
        scene->update();
        if (!m_directCreation && m_drawArea && m_drawArea->viewport()) {
            m_drawArea->viewport()->update();
            QApplication::processEvents();
        }
        
        m_executed = false;
        
        Logger::info(QString("CreateGraphicCommand: 撤销创建图形命令 - 类型: %1")
            .arg(static_cast<int>(m_type)));
    } catch (const std::exception& e) {
        Logger::error(QString("CreateGraphicCommand::undo: 异常 - %1").arg(e.what()));
        m_executed = false;
    } catch (...) {
        Logger::error("CreateGraphicCommand::undo: 未知异常");
        m_executed = false;
    }
}

QString CreateGraphicCommand::getDescription() const
{
    return QString("创建%1").arg(GraphicItem::graphicTypeToString(m_type));
}

QString CreateGraphicCommand::getType() const
{
    return "create";
} 