#include "transform_command.h"
#include "../core/graphic_item.h"
#include "../utils/logger.h"
#include <QTransform>

// 私有构造函数
TransformCommand::TransformCommand(QList<QGraphicsItem*> items, TransformType type)
    : m_items(items), m_transformType(type)
{
    saveOriginalStates();
}

// 创建旋转命令的静态方法
TransformCommand* TransformCommand::createRotateCommand(QList<QGraphicsItem*> items, double angle, QPointF center)
{
    TransformCommand* cmd = new TransformCommand(items, TransformType::Rotate);
    cmd->setRotationParams(angle, center);
    return cmd;
}

// 创建缩放命令的静态方法
TransformCommand* TransformCommand::createScaleCommand(QList<QGraphicsItem*> items, double factor, QPointF center)
{
    TransformCommand* cmd = new TransformCommand(items, TransformType::Scale);
    cmd->setScaleParams(factor, center);
    return cmd;
}

// 创建翻转命令的静态方法
TransformCommand* TransformCommand::createFlipCommand(QList<QGraphicsItem*> items, bool horizontal, QPointF center)
{
    TransformCommand* cmd = new TransformCommand(items, TransformType::Flip);
    cmd->setFlipParams(horizontal, center);
    return cmd;
}

// 设置旋转参数
void TransformCommand::setRotationParams(double angle, QPointF center)
{
    m_angle = angle;
    m_center = center;
}

// 设置缩放参数
void TransformCommand::setScaleParams(double factor, QPointF center)
{
    m_factor = factor;
    m_center = center;
}

// 设置翻转参数
void TransformCommand::setFlipParams(bool horizontal, QPointF center)
{
    m_isHorizontal = horizontal;
    m_center = center;
}

void TransformCommand::execute()
{
    if (m_executed) {
        return;
    }
    
    // 根据变换类型执行不同的操作
    if (m_transformType == TransformType::Rotate) {
        applyRotation();
    } else if (m_transformType == TransformType::Scale) {
        applyScaling();
    } else if (m_transformType == TransformType::Flip) {
        applyFlip();
    }
    
    m_executed = true;
}

void TransformCommand::undo()
{
    if (!m_executed) {
        return;
    }
    
    // 恢复所有项的原始状态
    for (QGraphicsItem* item : m_items) {
        if (item && m_originalStates.contains(item)) {
            ItemState state = m_originalStates[item];
            item->setPos(state.position);
            item->setRotation(state.rotation);
            
            // 设置缩放
            GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
            if (graphicItem) {
                graphicItem->setScale(state.scale);
            }
        }
    }
    
    m_executed = false;
}

QString TransformCommand::getDescription() const
{
    if (m_transformType == TransformType::Rotate) {
        return QString("旋转图形 %.1f 度").arg(m_angle);
    } else if (m_transformType == TransformType::Scale) {
        return QString("缩放图形 %.2f 倍").arg(m_factor);
    } else {
        return QString(m_isHorizontal ? "水平翻转图形" : "垂直翻转图形");
    }
}

QString TransformCommand::getType() const
{
    if (m_transformType == TransformType::Rotate) {
        return "rotate";
    } else if (m_transformType == TransformType::Scale) {
        return "scale";
    } else {
        return "flip";
    }
}

// 保存所有项的原始状态
void TransformCommand::saveOriginalStates()
{
    m_originalStates.clear();
    
    for (QGraphicsItem* item : m_items) {
        if (!item) continue;
        
        ItemState state;
        state.position = item->pos();
        state.rotation = item->rotation();
        
        // 获取图形的缩放
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (graphicItem) {
            state.scale = graphicItem->getScale();
        } else {
            state.scale = QPointF(1.0, 1.0);
        }
        
        m_originalStates[item] = state;
    }
}

// 应用旋转变换
void TransformCommand::applyRotation()
{
    for (QGraphicsItem* item : m_items) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (!graphicItem) continue;
        
        // 计算相对于中心点的旋转
        QPointF itemPos = item->pos();
        QPointF relativePos = itemPos - m_center;
        
        // 应用旋转变换
        QTransform transform;
        transform.translate(m_center.x(), m_center.y())
                .rotate(m_angle)
                .translate(-m_center.x(), -m_center.y());
        
        QPointF newPos = transform.map(itemPos);
        item->setPos(newPos);
        
        // 旋转图形自身
        graphicItem->rotateBy(m_angle);
    }
}

// 应用缩放变换
void TransformCommand::applyScaling()
{
    for (QGraphicsItem* item : m_items) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (!graphicItem) continue;
        
        // 计算相对于中心点的缩放
        QPointF itemPos = item->pos();
        QPointF relativePos = itemPos - m_center;
        QPointF newRelativePos = relativePos * m_factor;
        QPointF newPos = m_center + newRelativePos;
        
        // 设置新位置
        item->setPos(newPos);
        
        // 缩放图形自身
        graphicItem->scaleBy(m_factor);
    }
}

// 应用翻转变换
void TransformCommand::applyFlip()
{
    for (QGraphicsItem* item : m_items) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (!graphicItem) continue;
        
        // 应用镜像变换
        graphicItem->mirror(m_isHorizontal);
    }
} 