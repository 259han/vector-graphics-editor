#ifndef CONNECTION_POINT_OVERLAY_H
#define CONNECTION_POINT_OVERLAY_H

#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsScene>
#include "connection_manager.h"

/**
 * @brief 连接点可视化覆盖层
 * 
 * 负责在场景上层绘制连接点，独立于其他图形元素
 */
class ConnectionPointOverlay : public QGraphicsItem {
public:
    explicit ConnectionPointOverlay(ConnectionManager* manager);
    ~ConnectionPointOverlay() override = default;
    
    // QGraphicsItem接口
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
    // 重写形状和碰撞检测方法，确保不拦截鼠标事件
    QPainterPath shape() const override;
    bool contains(const QPointF &point) const override;
    
    // 设置可见性
    void setConnectionPointsVisible(bool visible);
    void setHighlightedPoint(const ConnectionManager::ConnectionPoint& point);
    void clearHighlight();
    
    // 更新显示
    void updateOverlay();

private:
    ConnectionManager* m_connectionManager;
    bool m_visible;
    bool m_hasHighlight;
    ConnectionManager::ConnectionPoint m_highlightedPoint;
    
    void drawConnectionPoint(QPainter* painter, const ConnectionManager::ConnectionPoint& point, bool highlighted = false);
};

#endif // CONNECTION_POINT_OVERLAY_H 