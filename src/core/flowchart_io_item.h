#ifndef FLOWCHART_IO_ITEM_H
#define FLOWCHART_IO_ITEM_H

#include "flowchart_base_item.h"

/**
 * @brief 流程图输入/输出框类 - 平行四边形形状
 * 
 * 表示流程图中的输入/输出步骤，平行四边形形状
 */
class FlowchartIOItem : public FlowchartBaseItem {
public:
    FlowchartIOItem(const QPointF& position, const QSizeF& size = QSizeF(120, 60), bool isInput = true);
    ~FlowchartIOItem() override = default;
    
    // QGraphicsItem接口实现
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;
    bool contains(const QPointF& point) const override;
    
    // GraphicItem接口实现
    GraphicType getGraphicType() const override { return FLOWCHART_IO; }
    QPainterPath toPath() const override;
    void restoreFromPoints(const std::vector<QPointF>& points) override;
    
    // 设置/获取是否为输入框
    void setIsInput(bool isInput) { m_isInput = isInput; update(); }
    bool isInput() const { return m_isInput; }
    
protected:
    std::vector<QPointF> getDrawPoints() const override;
    
    // 计算倾斜偏移量
    qreal calculateSkewOffset() const {
        return qMin(m_size.height() * 0.2, m_size.width() * 0.3);
    }
    
private:
    QSizeF m_size;
    bool m_isInput; // true为输入框，false为输出框
};

#endif // FLOWCHART_IO_ITEM_H 