#include "ellipse_graphic_item.h"

EllipseGraphicItem::EllipseGraphicItem(const QPointF& center, double width, double height)
    : m_center(QPointF(0, 0)), m_width(std::max(1.0, width)), m_height(std::max(1.0, height))
{
    // 设置绘制策略为EllipseDrawStrategy
    m_drawStrategy = std::make_shared<EllipseDrawStrategy>();
    
    // 设置位置为椭圆中心
    setPos(center);
    
    // 设置默认画笔和画刷
    m_pen.setColor(Qt::black);
    m_pen.setWidth(1);
    m_brush = Qt::NoBrush; // 无填充
}

QRectF EllipseGraphicItem::boundingRect() const
{
    // 边界矩形，确保包含整个椭圆
    qreal extra = m_pen.width() + 2.0; // 额外的边距
    return QRectF(
        -m_width/2 - extra,
        -m_height/2 - extra,
        m_width + extra * 2,
        m_height + extra * 2
    );
}

std::vector<QPointF> EllipseGraphicItem::getDrawPoints() const
{
    // 提供给DrawStrategy的点集合
    // 对于椭圆，需要中心点和边界点
    return {
        m_center,  // 中心点
        QPointF(m_width/2, m_height/2)  // 半宽和半高，用于计算椭圆尺寸
    };
}

QPointF EllipseGraphicItem::getCenter() const
{
    // 返回全局坐标系中的中心点
    return pos();
}

void EllipseGraphicItem::setCenter(const QPointF& center)
{
    // 设置新的位置（中心点）
    setPos(center);
}

double EllipseGraphicItem::getWidth() const
{
    return m_width;
}

void EllipseGraphicItem::setWidth(double width)
{
    // 确保宽度有效
    m_width = std::max(1.0, width);
    update();
}

double EllipseGraphicItem::getHeight() const
{
    return m_height;
}

void EllipseGraphicItem::setHeight(double height)
{
    // 确保高度有效
    m_height = std::max(1.0, height);
    update();
}

void EllipseGraphicItem::setSize(double width, double height)
{
    // 确保尺寸有效
    m_width = std::max(1.0, width);
    m_height = std::max(1.0, height);
    update();
} 