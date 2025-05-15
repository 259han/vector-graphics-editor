#include "rectangle_graphic_item.h"
#include <QPainter>
#include "../utils/logger.h"

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
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 基于缩放后的尺寸计算左上角相对位置
    QPointF scaledTopLeft = QPointF(-scaledWidth/2, -scaledHeight/2);
    
    // 边界矩形，相对于图形项坐标系的原点
    // 增加一些边距确保能正确显示边框
    qreal extra = m_pen.width() + 5.0;
    return QRectF(
        scaledTopLeft.x() - extra,
        scaledTopLeft.y() - extra,
        scaledWidth + extra * 2,
        scaledHeight + extra * 2
    );
}

// 添加shape方法实现准确的矩形碰撞检测
QPainterPath RectangleGraphicItem::shape() const
{
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 基于缩放后的尺寸计算左上角相对位置
    QPointF scaledTopLeft = QPointF(-scaledWidth/2, -scaledHeight/2);
    
    // 创建矩形路径
    QPainterPath path;
    path.addRect(QRectF(
        scaledTopLeft.x(),
        scaledTopLeft.y(),
        scaledWidth,
        scaledHeight
    ));
    
    // 考虑画笔宽度的影响，使用strokePath扩展路径
    QPainterPathStroker stroker;
    stroker.setWidth(m_pen.width());
    return path.united(stroker.createStroke(path));
}

std::vector<QPointF> RectangleGraphicItem::getDrawPoints() const
{
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 基于缩放后的尺寸计算左上角相对位置
    QPointF scaledTopLeft = QPointF(-scaledWidth/2, -scaledHeight/2);
    
    // 提供给DrawStrategy的点集合
    // 对于矩形，需要左上角和右下角点
    return {
        scaledTopLeft,
        scaledTopLeft + QPointF(scaledWidth, scaledHeight)
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

void RectangleGraphicItem::setScale(const QPointF& scale)
{
    // 设置新的基础比例
    m_scale = scale;
    
    // 直接应用scale进行绘制，而不是改变m_size
    // 这样可以避免累积放大效应
    
    // 绘制时的尺寸会根据getDrawPoints()计算，因此不需要修改m_size
    
    // 设置QGraphicsItem的基础缩放为1.0，让我们自行处理缩放
    QGraphicsItem::setScale(1.0);
    
    Logger::debug(QString("RectangleGraphicItem::setScale - 设置为(%1, %2), 基础尺寸: %3x%4")
                 .arg(scale.x(), 0, 'f', 3)
                 .arg(scale.y(), 0, 'f', 3)
                 .arg(m_size.width())
                 .arg(m_size.height()));
    
    // 更新图形
    update();
}

void RectangleGraphicItem::setScale(qreal scale)
{
    // 使用相同的x和y缩放
    setScale(QPointF(scale, scale));
}

// 实现contains方法，判断点是否在矩形内
bool RectangleGraphicItem::contains(const QPointF& point) const
{
    // 将点从场景坐标转换为图形项的本地坐标
    QPointF localPoint = mapFromScene(point);
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 计算矩形边界（相对于图形项中心）
    double halfWidth = scaledWidth / 2.0;
    double halfHeight = scaledHeight / 2.0;
    
    // 扩展边界以包含画笔宽度
    double penWidth = m_pen.width();
    double tolerance = penWidth + 2.0; // 增加额外的容差
    
    // 检查点是否在扩展的矩形内
    return (localPoint.x() >= -halfWidth - tolerance && 
            localPoint.x() <= halfWidth + tolerance &&
            localPoint.y() >= -halfHeight - tolerance && 
            localPoint.y() <= halfHeight + tolerance);
} 