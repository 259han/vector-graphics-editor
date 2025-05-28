#include "connection_point_overlay.h"
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>

ConnectionPointOverlay::ConnectionPointOverlay(ConnectionManager* manager)
    : QGraphicsItem()
    , m_connectionManager(manager)
    , m_visible(false)
    , m_hasHighlight(false)
{
    // 确保覆盖层在最上层
    setZValue(1000);
    
    // 完全禁用交互功能
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
    
    // 禁用所有鼠标事件
    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptTouchEvents(false);
    
    // 设置为透明，不接受任何点击
    setOpacity(1.0);
}

QRectF ConnectionPointOverlay::boundingRect() const
{
    // 只有在真正需要绘制时才返回有效边界
    if (!m_connectionManager || (!m_visible && !m_hasHighlight)) {
        return QRectF(); // 返回空矩形，表示不需要绘制
    }
    
    // 计算实际需要的绘制区域
    if (m_hasHighlight) {
        // 只为高亮点返回小的边界矩形
        const double radius = 20.0; // 连接点绘制半径
        QPointF pos = m_highlightedPoint.scenePos;
        return QRectF(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2);
    }
    
    if (m_visible && m_connectionManager->isConnectionPointsVisible()) {
        // 计算所有可见连接点的边界
        const auto& connectionPointsData = m_connectionManager->getConnectionPointsData();
        QRectF totalBounds;
        bool first = true;
        
        for (auto it = connectionPointsData.begin(); it != connectionPointsData.end(); ++it) {
            const QList<ConnectionManager::ConnectionPoint>& points = it.value();
            for (const ConnectionManager::ConnectionPoint& point : points) {
                const double radius = 20.0;
                QRectF pointBounds(point.scenePos.x() - radius, 
                                  point.scenePos.y() - radius,
                                  radius * 2, radius * 2);
                
                if (first) {
                    totalBounds = pointBounds;
                    first = false;
                } else {
                    totalBounds = totalBounds.united(pointBounds);
                }
            }
        }
        
        return first ? QRectF() : totalBounds;
    }
    
    return QRectF(); // 默认返回空矩形
}

QPainterPath ConnectionPointOverlay::shape() const
{
    // 返回空路径，确保不会拦截任何鼠标事件
    return QPainterPath();
}

bool ConnectionPointOverlay::contains(const QPointF &point) const
{
    Q_UNUSED(point)
    // 始终返回false，确保不会拦截任何鼠标事件
    return false;
}

void ConnectionPointOverlay::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    if (!m_connectionManager) {
        return;
    }
    
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    // 绘制所有可见的连接点
    if (m_connectionManager->isConnectionPointsVisible()) {
        const auto& connectionPointsData = m_connectionManager->getConnectionPointsData();
        
        // 遍历所有注册的流程图元素的连接点
        for (auto it = connectionPointsData.begin(); it != connectionPointsData.end(); ++it) {
            const QList<ConnectionManager::ConnectionPoint>& points = it.value();
            
            for (const ConnectionManager::ConnectionPoint& point : points) {
                // 检查是否为高亮点
                bool isHighlighted = m_hasHighlight && 
                                   m_highlightedPoint.item == point.item && 
                                   m_highlightedPoint.index == point.index;
                
                drawConnectionPoint(painter, point, isHighlighted);
            }
        }
    }
    
    // 绘制单独的高亮点（如果不在当前可见元素上）
    if (m_hasHighlight && !m_connectionManager->isConnectionPointsVisible()) {
        drawConnectionPoint(painter, m_highlightedPoint, true);
    }
}

void ConnectionPointOverlay::setConnectionPointsVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        update();
    }
}

void ConnectionPointOverlay::setHighlightedPoint(const ConnectionManager::ConnectionPoint& point)
{
    // 检查是否是同一个连接点，避免重复更新
    if (m_hasHighlight && 
        m_highlightedPoint.item == point.item && 
        m_highlightedPoint.index == point.index) {
        return; // 已经高亮了同一个点，无需更新
    }
    
    m_highlightedPoint = point;
    m_hasHighlight = true;
    
    // 只更新连接点周围的小区域而不是整个覆盖层
    const double updateRadius = 20.0; // 连接点更新半径
    QRectF updateRect(point.scenePos.x() - updateRadius, 
                     point.scenePos.y() - updateRadius,
                     updateRadius * 2, updateRadius * 2);
    update(updateRect);
}

void ConnectionPointOverlay::clearHighlight()
{
    if (!m_hasHighlight) {
        return; // 已经没有高亮，无需清除
    }
    
    // 保存旧的高亮位置用于局部更新
    QPointF oldHighlightPos = m_highlightedPoint.scenePos;
    
    m_hasHighlight = false;
    m_highlightedPoint = ConnectionManager::ConnectionPoint(); // 重置为空
    
    // 只更新之前高亮点的区域
    const double updateRadius = 20.0;
    QRectF updateRect(oldHighlightPos.x() - updateRadius, 
                     oldHighlightPos.y() - updateRadius,
                     updateRadius * 2, updateRadius * 2);
    update(updateRect);
}

void ConnectionPointOverlay::updateOverlay()
{
    update();
}

void ConnectionPointOverlay::drawConnectionPoint(QPainter* painter, const ConnectionManager::ConnectionPoint& point, bool highlighted)
{
    if (!point.item) return;
    
    QPointF scenePos = point.scenePos;
    double size = m_connectionManager->getConnectionPointSize();
    
    // 设置样式
    if (highlighted) {
        painter->setPen(QPen(QColor(255, 120, 0), 3)); // 橙色高亮边框
        painter->setBrush(QBrush(QColor(255, 120, 0, 150))); // 半透明橙色填充
        size *= 1.5; // 高亮时放大
    } else {
        painter->setPen(QPen(Qt::white, 2)); // 白色边框
        painter->setBrush(QBrush(QColor(0, 120, 255, 180))); // 半透明蓝色填充
    }
    
    // 绘制圆形连接点
    QRectF rect(scenePos.x() - size/2, scenePos.y() - size/2, size, size);
    painter->drawEllipse(rect);
    
    // 如果连接点被占用，在中心绘制一个小点
    if (point.isOccupied) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(Qt::red));
        double dotSize = size * 0.3;
        QRectF dotRect(scenePos.x() - dotSize/2, scenePos.y() - dotSize/2, dotSize, dotSize);
        painter->drawEllipse(dotRect);
    }
} 