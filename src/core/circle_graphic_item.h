#ifndef CIRCLE_GRAPHIC_ITEM_H
#define CIRCLE_GRAPHIC_ITEM_H

#include "graphic_item.h"
#include "draw_strategy.h"

// 圆形图形项
class CircleGraphicItem : public GraphicItem {
public:
    CircleGraphicItem(const QPointF& center = QPointF(0, 0), double radius = 50.0);
    ~CircleGraphicItem() override = default;
    
    // 实现GraphicItem的虚函数
    QRectF boundingRect() const override;
    GraphicType getGraphicType() const override { return CIRCLE; }
    
    // 圆形特有的方法
    QPointF getCenter() const;
    void setCenter(const QPointF& center);
    double getRadius() const { return m_radius; }
    void setRadius(double radius);
    
protected:
    // 提供绘制点集合
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    double m_radius; // 半径
};

#endif // CIRCLE_GRAPHIC_ITEM_H 