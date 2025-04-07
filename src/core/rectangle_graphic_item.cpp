#include "rectangle_graphic_item.h"
#include <QPainter>

RectangleGraphicItem::RectangleGraphicItem(const QPointF& topLeft, const QSizeF& size)
{
    // 设置默认画笔和画刷
    m_pen.setColor(Qt::black);
    m_pen.setWidth(2);
    m_brush = Qt::NoBrush;
    
    // 设置绘制策略
    m_drawStrategy = std::make_shared<RectangleDrawStrategy>();
    // 确保DrawStrategy使用正确的画笔设置
    m_drawStrategy->setColor(m_pen.color());
    m_drawStrategy->setLineWidth(m_pen.width());
    
    // 确保矩形至少有最小尺寸
    QSizeF validSize(std::max(1.0, size.width()), std::max(1.0, size.height()));
    m_size = validSize;
    
    // 设置位置
    setPos(topLeft + QPointF(validSize.width()/2, validSize.height()/2)); // 设置中心点为位置
    
    // 相对于坐标系原点的偏移
    m_topLeft = QPointF(-validSize.width()/2, -validSize.height()/2);
}

QRectF RectangleGraphicItem::boundingRect() const
{
    // 边界矩形，相对于图形项坐标系的原点
    // 增加一些边距确保能正确显示边框
    qreal extra = m_pen.width() + 5.0;
    return QRectF(
        m_topLeft.x() - extra,
        m_topLeft.y() - extra,
        m_size.width() + extra * 2,
        m_size.height() + extra * 2
    );
}

std::vector<QPointF> RectangleGraphicItem::getDrawPoints() const
{
    // 提供给DrawStrategy的点集合
    // 对于矩形，需要左上角和右下角点
    return {
        m_topLeft,
        m_topLeft + QPointF(m_size.width(), m_size.height())
    };
}

QPointF RectangleGraphicItem::getTopLeft() const
{
    // 返回全局坐标系中的左上角点
    return pos() + m_topLeft;
}

void RectangleGraphicItem::setTopLeft(const QPointF& topLeft)
{
    // 更新位置，保持大小不变
    setPos(topLeft + QPointF(m_size.width()/2, m_size.height()/2));
    update();
}

QSizeF RectangleGraphicItem::getSize() const
{
    return m_size;
}

void RectangleGraphicItem::setSize(const QSizeF& size)
{
    // 确保矩形至少有最小尺寸
    QSizeF validSize(std::max(1.0, size.width()), std::max(1.0, size.height()));
    
    // 更新大小，保持左上角位置不变
    QPointF oldTopLeft = getTopLeft();
    m_size = validSize;
    
    // 重新计算中心点和左上角相对坐标
    setPos(oldTopLeft + QPointF(validSize.width()/2, validSize.height()/2));
    m_topLeft = QPointF(-validSize.width()/2, -validSize.height()/2);
    
    update();
} 