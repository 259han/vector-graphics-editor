#ifndef BEZIER_GRAPHIC_ITEM_H
#define BEZIER_GRAPHIC_ITEM_H

#include "graphic_item.h"
#include "draw_strategy.h"

// Bezier曲线图形项
class BezierGraphicItem : public GraphicItem {
public:
    // 构造函数接受控制点集合
    BezierGraphicItem(const std::vector<QPointF>& controlPoints = {QPointF(0, 0), QPointF(100, 0)});
    ~BezierGraphicItem() override = default;
    
    // 实现GraphicItem的虚函数
    QRectF boundingRect() const override;
    Graphic::GraphicType getGraphicType() const override { return Graphic::BEZIER; }
    
    // Bezier曲线特有的方法
    const std::vector<QPointF>& getControlPoints() const { return m_controlPoints; }
    void setControlPoints(const std::vector<QPointF>& controlPoints);
    void addControlPoint(const QPointF& point);
    void clearControlPoints();
    int controlPointCount() const { return m_controlPoints.size(); }
    
    // 获取曲线中特定控制点
    QPointF getControlPoint(int index) const;
    // 设置特定控制点
    void setControlPoint(int index, const QPointF& point);
    
protected:
    // 提供绘制点集合
    std::vector<QPointF> getDrawPoints() const override;
    
private:
    std::vector<QPointF> m_controlPoints; // 控制点集合（相对于图形项坐标系）
    // 更新图形项的位置和绑定矩形
    void updateGeometry();
};

#endif // BEZIER_GRAPHIC_ITEM_H
