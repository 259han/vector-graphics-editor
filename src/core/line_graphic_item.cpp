#include "line_graphic_item.h"
#include <QLineF>
#include <QPainter>

LineGraphicItem::LineGraphicItem(const QPointF& startPoint, const QPointF& endPoint)
{
    // 设置默认画笔
    m_pen.setColor(Qt::black);
    m_pen.setWidth(2);
    
    // 设置透明画刷（无填充）
    m_brush = Qt::NoBrush;
    
    // 设置绘制策略为LineDrawStrategy
    m_drawStrategy = std::make_shared<LineDrawStrategy>();
    // 确保DrawStrategy使用正确的画笔属性
    m_drawStrategy->setColor(m_pen.color());
    m_drawStrategy->setLineWidth(m_pen.width());
    
    // 设置起点和终点（全局坐标）
    QPointF center = (startPoint + endPoint) / 2;
    setPos(center);
    
    // 设置相对坐标（相对于中心点）
    m_startPoint = startPoint - center;
    m_endPoint = endPoint - center;
    
    // 确保线段至少有最小长度，防止点击时看不见
    if (QLineF(m_startPoint, m_endPoint).length() < 1.0) {
        m_endPoint = m_startPoint + QPointF(1.0, 0);
    }
}

QRectF LineGraphicItem::boundingRect() const
{
    // 计算边界矩形，确保包含整条线段
    // 包括线宽和选择区域的边距
    qreal extra = m_pen.width() + 10.0; // 额外边距，用于选择及显示线宽
    return QRectF(
        qMin(m_startPoint.x(), m_endPoint.x()) - extra,
        qMin(m_startPoint.y(), m_endPoint.y()) - extra,
        qAbs(m_endPoint.x() - m_startPoint.x()) + extra * 2,
        qAbs(m_endPoint.y() - m_startPoint.y()) + extra * 2
    );
}

std::vector<QPointF> LineGraphicItem::getDrawPoints() const
{
    // 提供给DrawStrategy的点集合
    // 对于直线，需要起点和终点
    return {m_startPoint, m_endPoint};
}

QPointF LineGraphicItem::getStartPoint() const
{
    // 返回全局坐标系中的起点
    return pos() + m_startPoint;
}

void LineGraphicItem::setStartPoint(const QPointF& startPoint)
{
    // 设置新的起点（全局坐标）
    QPointF newEndPoint = getEndPoint(); // 保存当前终点的全局坐标
    QPointF newCenter = (startPoint + newEndPoint) / 2;
    
    // 更新位置和相对坐标
    setPos(newCenter);
    m_startPoint = startPoint - newCenter;
    m_endPoint = newEndPoint - newCenter;
    
    // 确保线段至少有最小长度
    if (QLineF(m_startPoint, m_endPoint).length() < 1.0) {
        m_endPoint = m_startPoint + QPointF(1.0, 0);
    }
    
    // 确保DrawStrategy使用当前的画笔设置
    if (m_drawStrategy) {
        m_drawStrategy->setColor(m_pen.color());
        m_drawStrategy->setLineWidth(m_pen.width());
    }
    
    update();
}

QPointF LineGraphicItem::getEndPoint() const
{
    // 返回全局坐标系中的终点
    return pos() + m_endPoint;
}

void LineGraphicItem::setEndPoint(const QPointF& endPoint)
{
    // 设置新的终点（全局坐标）
    QPointF newStartPoint = getStartPoint(); // 保存当前起点的全局坐标
    QPointF newCenter = (newStartPoint + endPoint) / 2;
    
    // 更新位置和相对坐标
    setPos(newCenter);
    m_startPoint = newStartPoint - newCenter;
    m_endPoint = endPoint - newCenter;
    
    // 确保线段至少有最小长度
    if (QLineF(m_startPoint, m_endPoint).length() < 1.0) {
        m_endPoint = m_startPoint + QPointF(1.0, 0);
    }
    
    // 确保DrawStrategy使用当前的画笔设置
    if (m_drawStrategy) {
        m_drawStrategy->setColor(m_pen.color());
        m_drawStrategy->setLineWidth(m_pen.width());
    }
    
    update();
}

void LineGraphicItem::updateGeometry()
{
    // 更新中心点和相对坐标
    QPointF globalStart = getStartPoint();
    QPointF globalEnd = getEndPoint();
    QPointF newCenter = (globalStart + globalEnd) / 2;
    
    setPos(newCenter);
    m_startPoint = globalStart - newCenter;
    m_endPoint = globalEnd - newCenter;
} 