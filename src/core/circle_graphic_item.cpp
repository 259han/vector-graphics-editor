#include "circle_graphic_item.h"

CircleGraphicItem::CircleGraphicItem(const QPointF& center, double radius)
{
    // 设置默认画笔和画刷
    m_pen.setColor(Qt::black);
    m_pen.setWidth(2);
    m_brush = Qt::NoBrush;
    
    // 设置绘制策略
    m_drawStrategy = std::make_shared<CircleDrawStrategy>();
    // 确保DrawStrategy使用正确的画笔设置
    m_drawStrategy->setColor(m_pen.color());
    m_drawStrategy->setLineWidth(m_pen.width());
    
    // 设置中心和半径
    setPos(center);
    m_radius = std::max(1.0, radius); // 确保半径至少为1
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
    return {QPointF(0, 0), QPointF(m_radius, 0)};
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

void CircleGraphicItem::setRadius(double radius)
{
    m_radius = std::max(1.0, radius); // 确保半径至少为1
    update(); // 更新显示
} 