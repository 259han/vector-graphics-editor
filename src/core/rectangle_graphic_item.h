#ifndef RECTANGLE_GRAPHIC_ITEM_H
#define RECTANGLE_GRAPHIC_ITEM_H

#include "graphic_item.h"
#include "draw_strategy.h"

// 矩形图形项
class RectangleGraphicItem : public GraphicItem {
public:
    RectangleGraphicItem(const QPointF& topLeft = QPointF(0, 0), const QSizeF& size = QSizeF(100, 60));
    ~RectangleGraphicItem() override = default;
    
    // 实现GraphicItem的虚函数
    QRectF boundingRect() const override;
    Graphic::GraphicType getGraphicType() const override { return Graphic::RECTANGLE; }
    
    // 矩形特有的方法
    QPointF getTopLeft() const;
    void setTopLeft(const QPointF& topLeft);
    QSizeF getSize() const;
    void setSize(const QSizeF& size);
    
protected:
    // 提供绘制点集合
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    QPointF m_topLeft; // 左上角点（相对于图形项坐标系）
    QSizeF m_size;     // 尺寸
};

#endif // RECTANGLE_GRAPHIC_ITEM_H 