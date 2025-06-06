#ifndef FLOWCHART_CONNECTOR_ITEM_H
#define FLOWCHART_CONNECTOR_ITEM_H

#include "flowchart_base_item.h"
#include <QPainterPath>
#include <QUuid>

/**
 * @brief 流程图连接线类
 * 
 * 表示流程图元素之间的连接关系，支持多种连接线样式和箭头类型
 */
class FlowchartConnectorItem : public FlowchartBaseItem {
public:
    // 连接线类型
    enum ConnectorType {
        StraightLine,    // 直线
        BezierCurve,     // 贝塞尔曲线
        Polyline,         // 折线
    };
    
    // 箭头类型
    enum ArrowType {
        NoArrow,         // 无箭头
        SingleArrow,     // 单箭头
        DoubleArrow      // 双箭头
    };
    
    FlowchartConnectorItem(const QPointF& startPoint = QPointF(), 
                          const QPointF& endPoint = QPointF(),
                          ConnectorType type = StraightLine,
                          ArrowType arrowType = SingleArrow);
    ~FlowchartConnectorItem() override = default;
    
    // QGraphicsItem接口实现
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    
    // GraphicItem接口实现
    GraphicType getGraphicType() const override { return FLOWCHART_CONNECTOR; }
    QPainterPath toPath() const override;
    void restoreFromPoints(const std::vector<QPointF>& points) override;
    
    // 设置/获取连接类型
    void setConnectorType(ConnectorType type) { m_connectorType = type; updatePath(); }
    ConnectorType getConnectorType() const { return m_connectorType; }
    
    // 设置/获取箭头类型
    void setArrowType(ArrowType type) { m_arrowType = type; update(); }
    ArrowType getArrowType() const { return m_arrowType; }
    
    // 设置控制点（用于曲线和折线）
    void setControlPoints(const QList<QPointF>& points) { m_controlPoints = points; updatePath(); }
    QList<QPointF> getControlPoints() const { return m_controlPoints; }
    
    // 设置起点和终点
    void setStartPoint(const QPointF& point) { m_startPoint = point; updatePath(); }
    void setEndPoint(const QPointF& point) { m_endPoint = point; updatePath(); }
    QPointF getStartPoint() const { return m_startPoint; }
    QPointF getEndPoint() const { return m_endPoint; }
    
    // 连接关系处理
    void setStartItem(FlowchartBaseItem* item) { m_startItem = item; }
    void setEndItem(FlowchartBaseItem* item) { m_endItem = item; }
    FlowchartBaseItem* getStartItem() const { return m_startItem; }
    FlowchartBaseItem* getEndItem() const { return m_endItem; }
    
    void setStartPointIndex(int index) { m_startPointIndex = index; }
    void setEndPointIndex(int index) { m_endPointIndex = index; }
    int getStartPointIndex() const { return m_startPointIndex; }
    int getEndPointIndex() const { return m_endPointIndex; }
    
    // 序列化和反序列化
    void serialize(QDataStream& out) const override;
    void deserialize(QDataStream& in) override;
    
    // 连接关系解析
    void resolveConnections(const QHash<QUuid, FlowchartBaseItem*>& itemMap);
    bool needsConnectionResolution() const { return !m_pendingStartUuid.isNull() || !m_pendingEndUuid.isNull(); }
    
    // 补充声明：重写基类的纯虚函数，和私有路径生成函数
    std::vector<QPointF> getDrawPoints() const override;
    QPainterPath createStraightPath() const;
    QPainterPath createOrthogonalPath() const;
    QPainterPath createCurvePath() const;

    // 更新路径
    void updatePath();
    
protected:
    // 绘制箭头
    void drawArrow(QPainter* painter, const QPointF& point, const QPointF& direction);

private:
    // 连接线类型
    ConnectorType m_connectorType;
    ArrowType m_arrowType;
    
    // 起点和终点
    QPointF m_startPoint;
    QPointF m_endPoint;
    
    // 控制点（用于曲线和折线）
    QList<QPointF> m_controlPoints;
    
    // 路径
    QPainterPath m_path;
    
    // 箭头大小
    qreal m_arrowSize = 10.0;
    
    // 连接关系
    FlowchartBaseItem* m_startItem = nullptr;
    FlowchartBaseItem* m_endItem = nullptr;
    int m_startPointIndex = -1;
    int m_endPointIndex = -1;
    
    // 延迟连接解析
    QUuid m_pendingStartUuid;
    QUuid m_pendingEndUuid;
    bool m_needsConnectionResolution = false;
};

#endif // FLOWCHART_CONNECTOR_ITEM_H 