#ifndef FLOWCHART_START_END_ITEM_H
#define FLOWCHART_START_END_ITEM_H

#include "flowchart_base_item.h"

/**
 * @brief 流程图开始/结束框类 - 圆角矩形形状
 * 
 * 表示流程图中的起始和结束步骤，圆角矩形形状
 */
class FlowchartStartEndItem : public FlowchartBaseItem {
public:
    FlowchartStartEndItem(const QPointF& position, const QSizeF& size = QSizeF(120, 60), bool isStart = true);
    ~FlowchartStartEndItem() override = default;
    
    // QGraphicsItem接口实现
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    bool contains(const QPointF& point) const override;
    
    // GraphicItem接口实现
    GraphicType getGraphicType() const override { return FLOWCHART_START_END; }
    QPainterPath toPath() const override;
    void restoreFromPoints(const std::vector<QPointF>& points) override;
    
    // 设置/获取是否为开始节点
    void setIsStart(bool isStart) { m_isStart = isStart; update(); }
    bool isStart() const { return m_isStart; }
    
protected:
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    QSizeF m_size;
    bool m_isStart; // true为开始节点，false为结束节点
    qreal m_cornerRadius = 15.0; // 圆角半径
};

#endif // FLOWCHART_START_END_ITEM_H 