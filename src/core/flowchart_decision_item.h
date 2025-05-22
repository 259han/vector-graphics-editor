#ifndef FLOWCHART_DECISION_ITEM_H
#define FLOWCHART_DECISION_ITEM_H

#include "flowchart_base_item.h"

/**
 * @brief 流程图判断框类 - 菱形形状
 * 
 * 表示流程图中的判断步骤，菱形形状
 */
class FlowchartDecisionItem : public FlowchartBaseItem {
public:
    FlowchartDecisionItem(const QPointF& position, const QSizeF& size = QSizeF(120, 80));
    ~FlowchartDecisionItem() override = default;
    
    // QGraphicsItem接口实现
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    bool contains(const QPointF& point) const override;
    
    // GraphicItem接口实现
    GraphicType getGraphicType() const override { return FLOWCHART_DECISION; }
    QPainterPath toPath() const override;
    void restoreFromPoints(const std::vector<QPointF>& points) override;
    
protected:
    std::vector<QPointF> getDrawPoints() const override;
    std::vector<QPointF> calculateConnectionPoints() const override;
    
private:
    QSizeF m_size;
};

#endif // FLOWCHART_DECISION_ITEM_H 