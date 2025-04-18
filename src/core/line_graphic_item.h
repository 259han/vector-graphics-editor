#ifndef LINE_GRAPHIC_ITEM_H
#define LINE_GRAPHIC_ITEM_H

#include "graphic_item.h"
#include "draw_strategy.h"

// 直线图形项
class LineGraphicItem : public GraphicItem {
public:
    LineGraphicItem(const QPointF& startPoint = QPointF(0, 0), const QPointF& endPoint = QPointF(100, 0));
    ~LineGraphicItem() override = default;
    
    // 实现GraphicItem的虚函数
    QRectF boundingRect() const override;
    GraphicType getGraphicType() const override { return LINE; }
    
    // 直线特有的方法
    QPointF getStartPoint() const;
    void setStartPoint(const QPointF& startPoint);
    QPointF getEndPoint() const;
    void setEndPoint(const QPointF& endPoint);
    
protected:
    // 提供绘制点集合
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    QPointF m_startPoint; // 起点（相对于图形项坐标系）
    QPointF m_endPoint;   // 终点（相对于图形项坐标系）
    
    // 更新图形项的位置和内部点
    void updateGeometry();
};

#endif // LINE_GRAPHIC_ITEM_H 