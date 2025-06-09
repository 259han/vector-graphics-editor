#include "clip_command.h"
#include "../utils/logger.h"
#include "../utils/clip_algorithms.h"
#include <QApplication>

ClipCommand::ClipCommand(QGraphicsScene* scene, 
                         GraphicItem* item,
                         const QPainterPath& clipPath)
    : m_scene(scene),
      m_item(item),
      m_clipPath(clipPath)
{
    Logger::debug(QString("ClipCommand: 创建裁剪命令 - 图形类型: %1")
        .arg(static_cast<int>(item->getGraphicType())));
    
    // 保存原始数据用于撤销
    m_originalPoints = item->getClipboardPoints();
    
    // 尝试获取原始路径（如果有形状方法）
    QPainterPath emptyPath;
    m_originalPath = emptyPath;
    
    if (auto *graphicsItem = dynamic_cast<QAbstractGraphicsShapeItem*>(item)) {
        m_originalPath = graphicsItem->shape();
    }
}

ClipCommand::~ClipCommand()
{
    Logger::debug("ClipCommand: 销毁裁剪命令");
}

void ClipCommand::execute()
{
    if (m_executed || !m_scene || !m_item) {
        Logger::warning("ClipCommand::execute: 命令已执行或场景/图形项无效");
        return;
    }
    
    Logger::debug("ClipCommand::execute: 开始执行裁剪命令");
    
    // 调用图形项的裁剪方法
    bool clipResult = false;
    if (auto *graphicItem = dynamic_cast<GraphicItem*>(m_item)) {
        // 直接调用图形项的clip方法，已经实现了自定义算法的支持
        clipResult = graphicItem->clip(m_clipPath);
    }
    
    if (!clipResult) {
        Logger::error("ClipCommand::execute: 裁剪操作失败");
        return;
    }
    
    m_executed = true;
    
    m_scene->update();
    
    Logger::info(QString("ClipCommand::execute: 裁剪命令执行成功 - 图形类型: %1")
        .arg(static_cast<int>(m_item->getGraphicType())));
}

void ClipCommand::undo()
{
    if (!m_executed || !m_scene || !m_item) {
        Logger::debug("ClipCommand::undo: 命令未执行或图形项为空");
        return;
    }
    
    // 检查图形项是否在场景中，避免操作已删除的项目
    if (!m_item->scene() || m_item->scene() != m_scene) {
        Logger::warning("ClipCommand::undo: 图形项不在当前场景中，可能已被删除");
        m_executed = false;
        return;
    }
    
    try {
        // 恢复原始数据
        if (auto *graphicItem = dynamic_cast<GraphicItem*>(m_item)) {
            // 重置为原始点集或形状
            graphicItem->restoreFromPoints(m_originalPoints);
        }
        
        m_scene->update();
        
        m_executed = false;
        
        Logger::info(QString("ClipCommand: 撤销裁剪命令 - 图形类型: %1")
            .arg(static_cast<int>(m_item->getGraphicType())));
    } catch (const std::exception& e) {
        Logger::error(QString("ClipCommand::undo: 异常 - %1").arg(e.what()));
        m_executed = false;
    } catch (...) {
        Logger::error("ClipCommand::undo: 未知异常");
        m_executed = false;
    }
}

QString ClipCommand::getDescription() const
{
    return QString("裁剪%1").arg(m_item ? GraphicItem::graphicTypeToString(m_item->getGraphicType()) : "图形");
}

QString ClipCommand::getType() const
{
    return "clip";
} 