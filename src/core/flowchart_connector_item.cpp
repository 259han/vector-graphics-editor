#include "flowchart_connector_item.h"
#include <QtMath>
#include <QDataStream>
#include <QUuid>

FlowchartConnectorItem::FlowchartConnectorItem(const QPointF& startPoint, const QPointF& endPoint, 
                                             ConnectorType type, ArrowType arrowType)
    : FlowchartBaseItem(), 
      m_startPoint(startPoint), 
      m_endPoint(endPoint),
      m_connectorType(type),
      m_arrowType(arrowType)
{
    // 连接器不需要设置位置，使用场景坐标
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false); // 连接器本身不可直接移动
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    
    // 确保接受鼠标悬停事件
    setAcceptHoverEvents(true);
    
    // 默认样式
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setBrush(Qt::black); // 用于箭头
    
    // 默认连接点为空，会根据路径自动生成
    m_controlPoints.clear();
    
    // 初始化路径
    updatePath();
    
    // 默认文本为空
    setTextVisible(false);
}

QRectF FlowchartConnectorItem::boundingRect() const
{
    // 返回路径的边界矩形，添加额外空间用于箭头
    QRectF baseRect = m_path.boundingRect();
    return baseRect.adjusted(-m_arrowSize, -m_arrowSize, m_arrowSize, m_arrowSize);
}

void FlowchartConnectorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 使用缓存时调用基类方法
    if (m_cachingEnabled && !m_cacheInvalid) {
        GraphicItem::paint(painter, option, widget);
        return;
    }
    
    // 绘制路径
    painter->setPen(m_pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);
    
    // 绘制箭头
    if (m_arrowType == SingleArrow || m_arrowType == DoubleArrow) {
        // 获取路径上的最后一个点和倒数第二个点，用于绘制箭头
        qreal len = m_path.length();
        qreal arrowPos = len - 0.1; // 稍微偏移，避免在路径末端
        QPointF endPoint = m_path.pointAtPercent(1.0);
        QPointF beforeEndPoint = m_path.pointAtPercent(qMax(0.0, (len - m_arrowSize) / len));
        
        drawArrow(painter, beforeEndPoint, endPoint);
    }
    
    // 如果是双箭头，绘制起点的箭头
    if (m_arrowType == DoubleArrow) {
        QPointF startPoint = m_path.pointAtPercent(0.0);
        QPointF afterStartPoint = m_path.pointAtPercent(qMin(1.0, m_arrowSize / m_path.length()));
        
        drawArrow(painter, afterStartPoint, startPoint);
    }
    
    // 如果有文本，在路径中间绘制
    if (m_textVisible && !m_text.isEmpty()) {
        // 保存画笔状态
        painter->save();
        
        // 在路径中间绘制文本
        qreal percent = 0.5; // 路径的中点
        QPointF textPoint = m_path.pointAtPercent(percent);
        
        // 绘制背景
        QFontMetricsF fm(m_textFont);
        QRectF textRect = fm.boundingRect(m_text);
        textRect.moveCenter(textPoint);
        textRect.adjust(-5, -2, 5, 2);
        
        painter->fillRect(textRect, Qt::white);
        
        // 设置文本绘制属性
        painter->setFont(m_textFont);
        painter->setPen(m_textColor);
        
        // 绘制文本
        painter->drawText(textRect, Qt::AlignCenter, m_text);
        
        // 恢复画笔状态
        painter->restore();
    }
    
    // 如果被选中，绘制选择控制点
    if (option->state & QStyle::State_Selected) {
        // 在线段的起点、终点和控制点处绘制控制柄
        painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter->setBrush(Qt::white);
        
        // 起点
        painter->drawEllipse(m_startPoint, 5, 5);
        
        // 终点
        painter->drawEllipse(m_endPoint, 5, 5);
        
        // 控制点
        for (const QPointF& point : m_controlPoints) {
            painter->drawEllipse(point, 5, 5);
        }
    }
    
    // 更新缓存
    if (m_cachingEnabled) {
        updateCache(painter, option);
    }
}

QPainterPath FlowchartConnectorItem::shape() const
{
    // 创建一个宽一点的路径，便于选择
    QPainterPath path = m_path;
    QPainterPathStroker stroker;
    stroker.setWidth(8); // 使线条选择区域宽一些，便于用户选择
    return stroker.createStroke(path);
}

QPainterPath FlowchartConnectorItem::toPath() const
{
    // 返回图形的路径表示，用于裁剪等操作
    return m_path;
}

std::vector<QPointF> FlowchartConnectorItem::getDrawPoints() const
{
    // 返回图形的关键点，用于序列化等操作
    std::vector<QPointF> points;
    
    // 添加起点和终点，使用相对于连接线的坐标
    points.push_back(m_startPoint - pos());
    points.push_back(m_endPoint - pos());
    
    // 添加控制点，使用相对于连接线的坐标
    for (const QPointF& point : m_controlPoints) {
        points.push_back(point - pos());
    }
    
    return points;
}

void FlowchartConnectorItem::updatePath()
{
    // 根据连接类型创建路径
    switch (m_connectorType) {
        case ConnectorType::StraightLine:
            m_path = createStraightPath();
            break;
        case ConnectorType::Polyline:
            m_path = createOrthogonalPath();
            break;
        case ConnectorType::BezierCurve:
            m_path = createCurvePath();
            break;
    }
    
    // 更新缓存
    invalidateCache();
    update();
}

void FlowchartConnectorItem::drawArrow(QPainter* painter, const QPointF& start, const QPointF& end)
{
    // 计算箭头方向
    QLineF line(start, end);
    
    // 如果线段太短，不绘制箭头
    if (line.length() < 1.0)
        return;
    
    // 保存画笔状态
    painter->save();
    
    // 计算线条的角度（从水平方向开始的弧度）
    double angle = std::atan2(line.dy(), line.dx());
    
    // 确保箭头底边垂直于线条
    // 计算垂直于线条的方向（顺时针旋转90度）
    double perpAngle = angle + M_PI/2;
    
    // 设置箭头大小
    double arrowWidth = m_arrowSize * 0.8; // 箭头宽度
    double arrowHeight = m_arrowSize * 1.2; // 箭头高度
    
    // 计算垂直于线条的方向上的箭头两端点
    QPointF baseCenter = end - QPointF(cos(angle) * arrowHeight/2, sin(angle) * arrowHeight/2);
    QPointF baseLeft = baseCenter + QPointF(cos(perpAngle) * arrowWidth/2, sin(perpAngle) * arrowWidth/2);
    QPointF baseRight = baseCenter - QPointF(cos(perpAngle) * arrowWidth/2, sin(perpAngle) * arrowWidth/2);
    
    // 箭头尖端向前延伸，确保是一个完整的三角形
    QPointF tip = baseCenter + QPointF(cos(angle) * arrowHeight, sin(angle) * arrowHeight);
    
    // 绘制箭头
    QPolygonF arrowHead;
    arrowHead << tip << baseLeft << baseRight;
    
    painter->setBrush(m_brush);
    painter->drawPolygon(arrowHead);
    
    // 恢复画笔状态
    painter->restore();
}

QPainterPath FlowchartConnectorItem::createStraightPath() const
{
    // 创建直线路径
    QPainterPath path;
    path.moveTo(m_startPoint);
    path.lineTo(m_endPoint);
    return path;
}

QPainterPath FlowchartConnectorItem::createOrthogonalPath() const
{
    // 创建正交线路径（折线）
    QPainterPath path;
    path.moveTo(m_startPoint);
    
    // 如果有控制点，使用控制点创建路径
    if (!m_controlPoints.isEmpty()) {
        for (const QPointF& point : m_controlPoints) {
            path.lineTo(point);
        }
        path.lineTo(m_endPoint);
    } else {
        // 否则，自动创建正交线
        // 计算中点
        QPointF midPoint((m_startPoint.x() + m_endPoint.x()) / 2,
                         (m_startPoint.y() + m_endPoint.y()) / 2);
        
        // 根据起点和终点的位置，决定折线的形状
        if (qAbs(m_startPoint.x() - m_endPoint.x()) > qAbs(m_startPoint.y() - m_endPoint.y())) {
            // 水平方向距离大，先水平移动到中点，再垂直移动
            path.lineTo(QPointF(midPoint.x(), m_startPoint.y()));
            path.lineTo(QPointF(midPoint.x(), m_endPoint.y()));
        } else {
            // 垂直方向距离大，先垂直移动到中点，再水平移动
            path.lineTo(QPointF(m_startPoint.x(), midPoint.y()));
            path.lineTo(QPointF(m_endPoint.x(), midPoint.y()));
        }
        
        path.lineTo(m_endPoint);
    }
    
    return path;
}

QPainterPath FlowchartConnectorItem::createCurvePath() const
{
    // 创建曲线路径
    QPainterPath path;
    path.moveTo(m_startPoint);
    
    // 如果有控制点，使用控制点创建贝塞尔曲线
    if (m_controlPoints.size() >= 2) {
        path.cubicTo(m_controlPoints[0], m_controlPoints[1], m_endPoint);
    } else if (m_controlPoints.size() == 1) {
        path.quadTo(m_controlPoints[0], m_endPoint);
    } else {
        // 否则，自动创建控制点
        QPointF ctrl1(m_startPoint.x() + (m_endPoint.x() - m_startPoint.x()) / 3,
                     m_startPoint.y() + (m_endPoint.y() - m_startPoint.y()) / 6);
        QPointF ctrl2(m_startPoint.x() + 2 * (m_endPoint.x() - m_startPoint.x()) / 3,
                     m_endPoint.y() - (m_endPoint.y() - m_startPoint.y()) / 6);
        path.cubicTo(ctrl1, ctrl2, m_endPoint);
    }
    
    return path;
}

void FlowchartConnectorItem::restoreFromPoints(const std::vector<QPointF>& points)
{
    // 从点集恢复图形，用于撤销裁剪等操作
    if (points.size() >= 2) {
        // 恢复起点和终点
        m_startPoint = points[0];
        m_endPoint = points[1];
        
        // 恢复控制点
        m_controlPoints.clear();
        for (size_t i = 2; i < points.size(); ++i) {
            m_controlPoints.append(points[i]);
        }
        
        // 更新路径
        updatePath();
    }
}

void FlowchartConnectorItem::serialize(QDataStream& out) const
{
    // 先序列化基类
    FlowchartBaseItem::serialize(out);
    
    // 保存连接线类型和箭头类型
    out << static_cast<int>(m_connectorType);
    out << static_cast<int>(m_arrowType);
    
    // 保存起点和终点
    out << m_startPoint;
    out << m_endPoint;
    
    // 保存控制点
    out << m_controlPoints;
    
    // 保存连接关系（使用UUID代替指针）
    out << (m_startItem ? m_startItem->uuid() : QUuid())
        << m_startPointIndex
        << (m_endItem ? m_endItem->uuid() : QUuid())
        << m_endPointIndex;
}

void FlowchartConnectorItem::deserialize(QDataStream& in)
{
    // 先反序列化基类
    FlowchartBaseItem::deserialize(in);
    
    // 读取连接线类型和箭头类型
    int connectorType, arrowType;
    in >> connectorType >> arrowType;
    m_connectorType = static_cast<ConnectorType>(connectorType);
    m_arrowType = static_cast<ArrowType>(arrowType);
    
    // 读取起点和终点
    in >> m_startPoint;
    in >> m_endPoint;
    
    // 读取控制点
    in >> m_controlPoints;
    
    // 读取连接关系（使用UUID）
    QUuid startUuid, endUuid;
    in >> startUuid >> m_startPointIndex
       >> endUuid >> m_endPointIndex;
    
    // 延迟绑定
    m_pendingStartUuid = startUuid;
    m_pendingEndUuid = endUuid;
    
    // 更新路径
    updatePath();
}

void FlowchartConnectorItem::resolveConnections(const QHash<QUuid, FlowchartBaseItem*>& itemMap)
{
    if (!m_pendingStartUuid.isNull()) {
        m_startItem = itemMap.value(m_pendingStartUuid, nullptr);
    }
    if (!m_pendingEndUuid.isNull()) {
        m_endItem = itemMap.value(m_pendingEndUuid, nullptr);
    }
    updatePath(); // 更新连接线位置
} 