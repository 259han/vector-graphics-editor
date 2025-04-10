#include "ellipse_graphic_item.h"
#include "../utils/logger.h"

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
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_width * m_scale.x();
    double scaledHeight = m_height * m_scale.y();
    
    // 边界矩形，确保包含整个椭圆
    qreal extra = m_pen.width() + 2.0; // 额外的边距
    return QRectF(
        -scaledWidth/2 - extra,
        -scaledHeight/2 - extra,
        scaledWidth + extra * 2,
        scaledHeight + extra * 2
    );
}

std::vector<QPointF> EllipseGraphicItem::getDrawPoints() const
{
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_width * m_scale.x();
    double scaledHeight = m_height * m_scale.y();
    
    // 提供给DrawStrategy的点集合
    // 对于椭圆，需要左上角和右下角点来定义包围矩形
    return {
        QPointF(-scaledWidth/2, -scaledHeight/2),  // 左上角点
        QPointF(scaledWidth/2, scaledHeight/2)     // 右下角点
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

void EllipseGraphicItem::setScale(const QPointF& scale)
{
    // 设置新的基础比例
    m_scale = scale;
    
    // 直接应用scale进行绘制，而不是改变m_width和m_height
    // 这样可以避免累积放大效应
    
    // 绘制时的宽高会根据getDrawPoints()计算，因此不需要修改m_width和m_height
    
    // 设置QGraphicsItem的基础缩放为1.0，让我们自行处理缩放
    QGraphicsItem::setScale(1.0);
    
    Logger::debug(QString("EllipseGraphicItem::setScale - 设置为(%1, %2), 基础尺寸: %3x%4")
                 .arg(scale.x(), 0, 'f', 3)
                 .arg(scale.y(), 0, 'f', 3)
                 .arg(m_width)
                 .arg(m_height));
    
    // 更新图形
    update();
}

void EllipseGraphicItem::setScale(qreal scale)
{
    // 使用相同的x和y缩放
    setScale(QPointF(scale, scale));
} 