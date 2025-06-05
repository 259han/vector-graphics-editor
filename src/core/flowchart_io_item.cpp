#include "flowchart_io_item.h"
#include "../utils/logger.h"
#include <QPainterPathStroker>

FlowchartIOItem::FlowchartIOItem(const QPointF& position, const QSizeF& size, bool isInput)
    : FlowchartBaseItem(), m_size(size), m_isInput(isInput)
{
    // 设置位置
    setPos(position);
    
    // 输入/输出框默认样式
    setPen(QPen(Qt::black, 2));
    setBrush(QBrush(QColor(255, 255, 255)));
    
    // 确保移动相关标志设置正确
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setMovable(true);
    
    // 确保接受鼠标悬停事件
    setAcceptHoverEvents(true);
    
    // 默认文本
    setText(m_isInput ? "输入" : "输出");
}

QRectF FlowchartIOItem::boundingRect() const
{
    // 返回图元的边界矩形，考虑到倾斜因素，需要适当增加宽度
    qreal skewOffset = calculateSkewOffset();
    return QRectF(-m_size.width() / 2 - skewOffset/2, -m_size.height() / 2, 
                  m_size.width() + skewOffset, m_size.height());
}

void FlowchartIOItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 使用缓存时调用基类方法
    if (m_cachingEnabled && !m_cacheInvalid) {
        GraphicItem::paint(painter, option, widget);
        return;
    }
    
    // 绘制平行四边形
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    
    QRectF rect = boundingRect();
    qreal skewOffset = calculateSkewOffset();
    
    QPolygonF parallelogram;
    
    // 平行四边形的四个顶点
    parallelogram << QPointF(rect.left() + skewOffset, rect.top())   // 左上
                 << QPointF(rect.right(), rect.top())                // 右上
                 << QPointF(rect.right() - skewOffset, rect.bottom()) // 右下
                 << QPointF(rect.left(), rect.bottom());             // 左下
    
    painter->drawPolygon(parallelogram);
    
    // 绘制文本
    drawText(painter, rect);
    
    // 如果被选中，绘制选择控制点
    if (option->state & QStyle::State_Selected) {
        drawSelectionHandles(painter);
    }
    
    // 更新缓存
    if (m_cachingEnabled) {
        updateCache(painter, option);
    }
}

QPainterPath FlowchartIOItem::shape() const
{
    // 创建平行四边形路径
    QPainterPath path;
    QRectF rect = boundingRect();
    qreal skewOffset = calculateSkewOffset();
    
    // 平行四边形的四个顶点
    path.moveTo(rect.left() + skewOffset, rect.top());   // 左上
    path.lineTo(rect.right(), rect.top());              // 右上
    path.lineTo(rect.right() - skewOffset, rect.bottom()); // 右下
    path.lineTo(rect.left(), rect.bottom());            // 左下
    path.closeSubpath();
    
    // 根据尺寸决定容差大小
    qreal minDimension = qMin(m_size.width(), m_size.height());
    qreal tolerance = m_pen.width() + 10.0; // 大幅增加基础容差
    
    // 对小尺寸图形使用更大的容差
    if (minDimension < 150) {
        tolerance = qMax(tolerance, 20.0 + 100.0/minDimension); // 大幅增加容差和系数
    }
    
    // 考虑画笔宽度的影响，使用strokePath扩展路径
    QPainterPathStroker stroker;
    stroker.setWidth(tolerance);
    return path.united(stroker.createStroke(path));
}

// 重写contains方法，实现更准确的平行四边形内部检测
bool FlowchartIOItem::contains(const QPointF& point) const
{
    // 将点从场景坐标转换为图形项的本地坐标
    QPointF localPoint = mapFromScene(point);
    
    // 对于小尺寸图形，使用更大的容差
    qreal minDimension = qMin(m_size.width(), m_size.height());
    QPainterPath path = shape();
    
    // 使用固定的超大容差值1500
    // 创建一个扩展的矩形区域用于检测
    QPainterPathStroker stroker;
    qreal extraTolerance = 1500.0; // 设置为固定的1500，与其他图形保持一致
    stroker.setWidth(extraTolerance);
    QPainterPath expandedPath = path.united(stroker.createStroke(path));
    return expandedPath.contains(localPoint);
    
    // 注释掉原有代码
    /*
    // 大幅增加容差，确保小尺寸图形也容易被选中
    // 如果图形较小，则增加额外的检测区域
    if (minDimension < 200) {  // 大幅提高阈值
        // 创建一个扩展的矩形区域用于检测
        QPainterPathStroker stroker;
        qreal extraTolerance = qMax(50.0, 100.0 - minDimension/4.0); // 大幅增加基础容差和系数
        stroker.setWidth(extraTolerance);
        QPainterPath expandedPath = path.united(stroker.createStroke(path));
        return expandedPath.contains(localPoint);
    }
    
    // 使用普通检测方法
    return path.contains(localPoint);
    */
}

QPainterPath FlowchartIOItem::toPath() const
{
    // 返回图形的路径表示，用于裁剪等操作
    return shape();
}

std::vector<QPointF> FlowchartIOItem::getDrawPoints() const
{
    // 返回图形的顶点，用于序列化等操作
    std::vector<QPointF> points;
    QRectF rect = boundingRect();
    
    // 返回中心点
    points.push_back(rect.center());
    
    // 返回大小点（中心点 + 半宽高）
    points.push_back(QPointF(
        rect.center().x() + rect.width() / 2,
        rect.center().y() + rect.height() / 2
    ));
    
    return points;
}

void FlowchartIOItem::restoreFromPoints(const std::vector<QPointF>& points)
{
    Logger::debug("FlowchartIOItem::restoreFromPoints: 开始恢复输入/输出框形状和大小");

    if (points.empty()) {
        Logger::warning("FlowchartIOItem::restoreFromPoints: 点集为空，无法恢复形状和大小");
        return;
    }

    // 调用基类方法设置位置
    FlowchartBaseItem::restoreFromPoints(points);

    // 如果只有1个点，使用默认大小
    if (points.size() == 1) {
        m_size = QSizeF(120, 60);
        Logger::debug("FlowchartIOItem::restoreFromPoints: 恢复输入/输出框形状成功(1点，默认大小)");
    } else {
        // 使用第二个点计算大小
        QPointF center = points[0];
        QPointF sizePoint = points[1];
        m_size = QSizeF(std::abs(sizePoint.x() - center.x()) * 2, 
                       std::abs(sizePoint.y() - center.y()) * 2);
        
        Logger::debug(QString("FlowchartIOItem::restoreFromPoints: 恢复输入/输出框形状成功(多点) - 中心=(%1,%2), 大小=(%3,%4)")
            .arg(center.x()).arg(center.y())
            .arg(m_size.width()).arg(m_size.height()));
    }
    
    invalidateCache();
    update();
} 