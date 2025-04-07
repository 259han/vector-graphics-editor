#include "circle_graphic_item.h"

CircleGraphicItem::CircleGraphicItem(const QPointF& center, double radius)
    : m_center(center), m_radius(radius)
{
    // 设置绘制策略为CircleDrawStrategy
    m_drawStrategy = std::make_shared<CircleDrawStrategy>();
    
    // 设置位置
    setPos(center);
    m_center = QPointF(0, 0); // 相对于图形项坐标系，中心在原点
}

QRectF CircleGraphicItem::boundingRect() const
{
    // 边界矩形，确保包含整个圆形
    return QRectF(-m_radius, -m_radius, m_radius * 2, m_radius * 2);
}

std::vector<QPointF> CircleGraphicItem::getDrawPoints() const
{
    // 提供给DrawStrategy的点集合
    // 对于圆形，需要一个中心点和一个确定半径的点
    return {m_center, QPointF(m_center.x() + m_radius, m_center.y())};
}

QPointF CircleGraphicItem::getCenter() const
{
    // 返回全局坐标系中的中心点
    return pos();
}

void CircleGraphicItem::setCenter(const QPointF& center)
{
    // 设置新的位置（中心点）
    setPos(center);
}

double CircleGraphicItem::getRadius() const
{
    return m_radius;
}

void CircleGraphicItem::setRadius(double radius)
{
    m_radius = radius;
    update(); // 更新显示
} 