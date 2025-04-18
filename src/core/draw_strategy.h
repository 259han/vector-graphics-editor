#ifndef DRAW_STRATEGY_H
#define DRAW_STRATEGY_H

#include <QPainter>
#include <QPointF>
#include <vector>
#include <QPainterPath>
#include <map>
#include <utility> 

class DrawStrategy {
public:
    virtual ~DrawStrategy() = default;
    virtual void draw(QPainter* painter, const std::vector<QPointF>& points) = 0;
    virtual void setColor(const QColor& color) = 0;
    virtual void setLineWidth(int width) = 0;

protected:
    QColor m_color = Qt::black;
    int m_lineWidth = 2;
};

class LineDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter* painter, const std::vector<QPointF>& points) override;
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
};

class RectangleDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter* painter, const std::vector<QPointF>& points) override;
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
};

class CircleDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter* painter, const std::vector<QPointF>& points) override;
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
};

class EllipseDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter* painter, const std::vector<QPointF>& points) override;
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
};

class BezierDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter* painter, const std::vector<QPointF>& points) override;
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
    
    // 计算n阶Bezier曲线上的点
    QPointF calculateBezierPoint(const std::vector<QPointF>& controlPoints, double t) const;
private:
    

};

#endif // DRAW_STRATEGY_H