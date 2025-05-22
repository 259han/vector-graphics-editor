#include "flowchart_decision_item.h"
#include <QPainterPathStroker>

FlowchartDecisionItem::FlowchartDecisionItem(const QPointF& position, const QSizeF& size)
    : FlowchartBaseItem(), m_size(size)
{
    // 设置位置
    setPos(position);
    
    // 判断框默认样式
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
    setText("判断?");
}

QRectF FlowchartDecisionItem::boundingRect() const
{
    // 返回图元的边界矩形，注意菱形需要足够空间
    return QRectF(-m_size.width() / 2, -m_size.height() / 2, m_size.width(), m_size.height());
}

void FlowchartDecisionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 使用缓存时调用基类方法
    if (m_cachingEnabled && !m_cacheInvalid) {
        GraphicItem::paint(painter, option, widget);
        return;
    }
    
    // 绘制菱形
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    
    QRectF rect = boundingRect();
    QPolygonF diamond;
    
    // 菱形的四个顶点
    diamond << QPointF(rect.center().x(), rect.top())              // 上
           << QPointF(rect.right(), rect.center().y())             // 右
           << QPointF(rect.center().x(), rect.bottom())            // 下
           << QPointF(rect.left(), rect.center().y());             // 左
    
    painter->drawPolygon(diamond);
    
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

QPainterPath FlowchartDecisionItem::shape() const
{
    // 创建菱形路径
    QPainterPath path;
    QRectF rect = boundingRect();
    
    // 菱形的四个顶点
    path.moveTo(rect.center().x(), rect.top());
    path.lineTo(rect.right(), rect.center().y());
    path.lineTo(rect.center().x(), rect.bottom());
    path.lineTo(rect.left(), rect.center().y());
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

// 重写contains方法，实现更准确的菱形内部检测
bool FlowchartDecisionItem::contains(const QPointF& point) const
{
    // 将点从场景坐标转换为图形项的本地坐标
    QPointF localPoint = mapFromScene(point);
    
    // 对于小尺寸图形，使用更大的容差
    qreal minDimension = qMin(m_size.width(), m_size.height());
    QPainterPath path = shape();
    
    // 使用固定的超大容差值1500
    // 创建一个扩展的矩形区域用于检测
    QPainterPathStroker stroker;
    qreal extraTolerance = 1500.0; // 设置为固定的1500，与处理框保持一致
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

QPainterPath FlowchartDecisionItem::toPath() const
{
    // 返回图形的路径表示，用于裁剪等操作
    return shape();
}

std::vector<QPointF> FlowchartDecisionItem::getDrawPoints() const
{
    // 返回图形的顶点，用于序列化等操作
    std::vector<QPointF> points;
    QRectF rect = boundingRect();
    
    // 菱形的四个顶点
    points.push_back(QPointF(rect.center().x(), rect.top()));      // 上
    points.push_back(QPointF(rect.right(), rect.center().y()));    // 右
    points.push_back(QPointF(rect.center().x(), rect.bottom()));   // 下
    points.push_back(QPointF(rect.left(), rect.center().y()));     // 左
    
    return points;
}

std::vector<QPointF> FlowchartDecisionItem::calculateConnectionPoints() const
{
    // 返回菱形的四个顶点作为连接点
    std::vector<QPointF> points;
    QRectF rect = boundingRect();
    
    // 菱形的四个顶点
    points.push_back(QPointF(rect.center().x(), rect.top()));      // 上
    points.push_back(QPointF(rect.right(), rect.center().y()));    // 右
    points.push_back(QPointF(rect.center().x(), rect.bottom()));   // 下
    points.push_back(QPointF(rect.left(), rect.center().y()));     // 左
    
    // 转换为场景坐标
    for (auto& point : points) {
        point = mapToScene(point);
    }
    
    return points;
}

void FlowchartDecisionItem::restoreFromPoints(const std::vector<QPointF>& points)
{
    // 从点集恢复图形，用于撤销裁剪等操作
    if (points.size() >= 4) {
        // 计算边界矩形
        qreal minX = std::numeric_limits<qreal>::max();
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxX = std::numeric_limits<qreal>::lowest();
        qreal maxY = std::numeric_limits<qreal>::lowest();
        
        for (const auto& point : points) {
            minX = std::min(minX, point.x());
            minY = std::min(minY, point.y());
            maxX = std::max(maxX, point.x());
            maxY = std::max(maxY, point.y());
        }
        
        // 更新大小
        m_size = QSizeF(maxX - minX, maxY - minY);
        
        // 更新缓存
        invalidateCache();
        update();
    }
} 