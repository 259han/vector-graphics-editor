#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <QPointF>
#include <QPainter>
#include <memory>
#include <vector>

class DrawStrategy;

class Graphic {
public:
    enum GraphicType {
        LINE,
        RECTANGLE, 
        CIRCLE,
        ELLIPSE,
        TRIANGLE,
        FLOWCHART_NODE,
        BEZIER,
        CUSTOM
    };

    Graphic() = default;
    virtual ~Graphic() = default;

    // 核心绘制方法
    virtual void draw(QPainter& painter) const = 0;
    
    // 几何变换方法
    virtual void move(const QPointF& offset) = 0;
    virtual void rotate(double angle) = 0;
    virtual void scale(double factor) = 0;

    // 属性获取方法
    virtual QPointF getCenter() const = 0;
    virtual GraphicType getType() const = 0;
    
    // 包围盒方法，用于选择和碰撞检测
    virtual QRectF getBoundingBox() const = 0;

    // 设置绘制策略
    virtual void setDrawStrategy(std::shared_ptr<DrawStrategy> strategy) = 0;

    // 克隆方法，用于复制图形
    virtual std::unique_ptr<Graphic> clone() const = 0;

    virtual void drawEllipse(const QPointF& center, double width, double height) = 0;

protected:
    std::shared_ptr<DrawStrategy> m_drawStrategy;
};

#endif // GRAPHIC_H