#ifndef ELLIPSE_GRAPHIC_ITEM_H
#define ELLIPSE_GRAPHIC_ITEM_H

#include "graphic_item.h"
#include "draw_strategy.h"

// 椭圆图形项
class EllipseGraphicItem : public GraphicItem {
public:
    EllipseGraphicItem(const QPointF& center = QPointF(0, 0), double width = 100.0, double height = 60.0);
    ~EllipseGraphicItem() override = default;
    
    // 实现GraphicItem的虚函数
    QRectF boundingRect() const override;
    GraphicType getGraphicType() const override { return ELLIPSE; }
    
    // 重写缩放方法
    void setScale(const QPointF& scale) override;
    void setScale(qreal scale) override;
    
    // 椭圆特有的方法
    QPointF getCenter() const;
    void setCenter(const QPointF& center);
    double getWidth() const;
    void setWidth(double width);
    double getHeight() const;
    void setHeight(double height);
    void setSize(double width, double height);
    
protected:
    // 提供绘制点集合
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    QPointF m_center; // 中心点（相对于图形项坐标系）
    double m_width;   // 宽度
    double m_height;  // 高度
};

#endif // ELLIPSE_GRAPHIC_ITEM_H 