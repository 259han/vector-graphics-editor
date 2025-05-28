#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <QObject>
#include <QGraphicsItem>
#include <QPointF>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QPainter>
#include <QGraphicsScene>
#include "flowchart_base_item.h"
#include "flowchart_connector_item.h"

/**
 * @brief 连接管理器 - 管理流程图元素之间的自动连接
 * 
 * 负责连接点的可视化、自动吸附、连接关系维护等功能
 */
class ConnectionManager : public QObject {
    Q_OBJECT

public:
    // 连接点信息结构
    struct ConnectionPoint {
        FlowchartBaseItem* item;        // 所属的流程图元素
        QPointF scenePos;               // 在场景中的位置
        QPointF localPos;               // 在元素中的本地位置
        int index;                      // 连接点索引
        bool isOccupied;                // 是否被占用
        
        ConnectionPoint() : item(nullptr), index(-1), isOccupied(false) {}
        ConnectionPoint(FlowchartBaseItem* i, const QPointF& scene, const QPointF& local, int idx) 
            : item(i), scenePos(scene), localPos(local), index(idx), isOccupied(false) {}
    };
    
    // 连接关系结构
    struct Connection {
        FlowchartBaseItem* fromItem;    // 起始元素
        int fromPointIndex;             // 起始连接点索引
        FlowchartBaseItem* toItem;      // 目标元素
        int toPointIndex;               // 目标连接点索引
        FlowchartConnectorItem* connector; // 连接器对象
        
        Connection() : fromItem(nullptr), fromPointIndex(-1), toItem(nullptr), toPointIndex(-1), connector(nullptr) {}
        Connection(FlowchartBaseItem* from, int fromIdx, FlowchartBaseItem* to, int toIdx, FlowchartConnectorItem* conn)
            : fromItem(from), fromPointIndex(fromIdx), toItem(to), toPointIndex(toIdx), connector(conn) {}
    };

    explicit ConnectionManager(QGraphicsScene* scene, QObject* parent = nullptr);
    ~ConnectionManager() override;
    
    // 连接点管理
    void registerFlowchartItem(FlowchartBaseItem* item);
    void unregisterFlowchartItem(FlowchartBaseItem* item);
    void updateConnectionPoints(FlowchartBaseItem* item);
    void clearAllConnectionPoints();
    void cleanupInvalidItems();
    
    // 连接点可视化
    void showConnectionPoints(FlowchartBaseItem* item);
    void hideConnectionPoints();
    void setConnectionPointsVisible(bool visible);
    bool isConnectionPointsVisible() const { return m_connectionPointsVisible; }
    
    // 自动吸附功能
    QPointF findNearestConnectionPoint(const QPointF& scenePos, FlowchartBaseItem* excludeItem = nullptr);
    ConnectionPoint* findConnectionPointAt(const QPointF& scenePos, double tolerance = 15.0);
    bool isNearConnectionPoint(const QPointF& scenePos, double tolerance = 15.0);
    
    // 连接管理
    bool createConnection(FlowchartBaseItem* fromItem, int fromPointIndex, 
                         FlowchartBaseItem* toItem, int toPointIndex,
                         FlowchartConnectorItem::ConnectorType connectorType = FlowchartConnectorItem::StraightLine,
                         FlowchartConnectorItem::ArrowType arrowType = FlowchartConnectorItem::SingleArrow);
    
    void removeConnection(FlowchartConnectorItem* connector);
    void removeAllConnectionsFor(FlowchartBaseItem* item);
    
    // 连接器更新
    void updateConnections(FlowchartBaseItem* item);
    void updateAllConnections();
    
    // 连接验证
    bool canConnect(FlowchartBaseItem* fromItem, FlowchartBaseItem* toItem) const;
    bool isConnected(FlowchartBaseItem* fromItem, FlowchartBaseItem* toItem) const;
    
    // 获取连接信息
    QList<Connection> getConnectionsFor(FlowchartBaseItem* item) const;
    QList<Connection> getAllConnections() const { return m_connections; }
    
    // 连接点高亮
    void highlightConnectionPoint(const ConnectionPoint& point);
    void clearHighlight();
    
    // 用于ConnectionPointOverlay访问连接点数据
    const QMap<FlowchartBaseItem*, QList<ConnectionPoint>>& getConnectionPointsData() const { return m_connectionPoints; }
    
    // 设置参数
    void setSnapTolerance(double tolerance) { m_snapTolerance = tolerance; }
    double getSnapTolerance() const { return m_snapTolerance; }
    
    void setConnectionPointSize(double size) { m_connectionPointSize = size; }
    double getConnectionPointSize() const { return m_connectionPointSize; }

signals:
    void connectionCreated(FlowchartBaseItem* fromItem, FlowchartBaseItem* toItem, FlowchartConnectorItem* connector);
    void connectionRemoved(FlowchartConnectorItem* connector);
    void connectionPointHovered(const ConnectionPoint& point);

private slots:
    void onItemPositionChanged();
    void onUpdateTimer();

private:
    QGraphicsScene* m_scene;
    
    // 连接点数据
    QMap<FlowchartBaseItem*, QList<ConnectionPoint>> m_connectionPoints;
    QList<Connection> m_connections;
    
    // 可视化状态
    bool m_connectionPointsVisible;
    FlowchartBaseItem* m_currentVisibleItem;
    
    // 高亮状态
    ConnectionPoint m_highlightedPoint;
    bool m_hasHighlight;
    
    // 参数设置
    double m_snapTolerance;          // 自动吸附容差
    double m_connectionPointSize;    // 连接点显示大小
    
    // 更新控制
    QTimer* m_updateTimer;
    QList<FlowchartBaseItem*> m_itemsToUpdate;
    
    // 性能优化缓存
    QMap<FlowchartBaseItem*, QRectF> m_lastItemBounds;  // 缓存元素的上次边界矩形
    QMap<FlowchartBaseItem*, int> m_lastConnectionCount; // 缓存上次连接点数量
    
    // 内部方法
    void calculateConnectionPoints(FlowchartBaseItem* item);
    void drawConnectionPoints(QPainter* painter);
    void drawHighlight(QPainter* painter);
    
    // 连接点样式
    QColor m_connectionPointColor;
    QColor m_highlightColor;
    QPen m_connectionPointPen;
    QBrush m_connectionPointBrush;
};

#endif // CONNECTION_MANAGER_H 