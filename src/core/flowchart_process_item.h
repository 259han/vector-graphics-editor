#ifndef FLOWCHART_PROCESS_ITEM_H
#define FLOWCHART_PROCESS_ITEM_H

#include "flowchart_base_item.h"

/**
 * @brief 流程图处理框类 - 矩形形状
 * 
 * 表示流程图中的处理步骤，标准矩形形状
 */
class FlowchartProcessItem : public FlowchartBaseItem {
public:
    FlowchartProcessItem(const QPointF& position, const QSizeF& size = QSizeF(120, 60));
    ~FlowchartProcessItem() override = default;
    
    // QGraphicsItem接口实现
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    bool contains(const QPointF& point) const override;
    
    // GraphicItem接口实现
    GraphicType getGraphicType() const override { return FLOWCHART_PROCESS; }
    QPainterPath toPath() const override;
    void restoreFromPoints(const std::vector<QPointF>& points) override;
    
protected:
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    QSizeF m_size;
};

#endif // FLOWCHART_PROCESS_ITEM_H 