#ifndef FLOWCHART_CONNECTOR_ITEM_H
#define FLOWCHART_CONNECTOR_ITEM_H

#include "flowchart_base_item.h"

/**
 * @brief 流程图连接器类 - 带箭头的线
 * 
 * 表示流程图中的连接关系，带箭头的线条
 */
class FlowchartConnectorItem : public FlowchartBaseItem {
public:
    // 连接类型枚举
    enum ConnectorType {
        StraightLine,   // 直线
        OrthogonalLine, // 正交线（折线）
        CurveLine       // 曲线
    };
    
    // 箭头类型枚举
    enum ArrowType {
        NoArrow,      // 无箭头
        SingleArrow,  // 单箭头（起点到终点）
        DoubleArrow   // 双箭头（两端都有）
    };
    
    FlowchartConnectorItem(const QPointF& startPoint, const QPointF& endPoint, 
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
    
protected:
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    QPointF m_startPoint;
    QPointF m_endPoint;
    ConnectorType m_connectorType;
    ArrowType m_arrowType;
    QList<QPointF> m_controlPoints; // 用于曲线和折线的控制点
    QPainterPath m_path; // 缓存的路径
    
    // 箭头参数
    qreal m_arrowSize = 10.0;
    
    // 更新路径
    void updatePath();
    
    // 绘制箭头
    void drawArrow(QPainter* painter, const QPointF& start, const QPointF& end);
    
    // 生成各种类型的路径
    QPainterPath createStraightPath() const;
    QPainterPath createOrthogonalPath() const;
    QPainterPath createCurvePath() const;
};

#endif // FLOWCHART_CONNECTOR_ITEM_H 