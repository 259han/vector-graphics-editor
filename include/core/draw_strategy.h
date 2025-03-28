#ifndef DRAW_STRATEGY_H
#define DRAW_STRATEGY_H

#include <QPainter>
#include <QPointF>
#include <vector>

class DrawStrategy {
public:
    virtual ~DrawStrategy() = default;
    virtual void draw(QPainter& painter, const std::vector<QPointF>& points) const = 0;
    virtual void setColor(const QColor& color) = 0;
    virtual void setLineWidth(int width) = 0;

protected:
    QColor m_color = Qt::black;
    int m_lineWidth = 2;
};

class LineDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override;
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
private:
    QColor m_color = Qt::black;
    int m_lineWidth = 2;
};

class RectangleDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override {
        if (points.size() < 2) return;
        painter.setPen(QPen(m_color, m_lineWidth));
        QRectF rect(points[0], points[1]);
        painter.drawRect(rect);
    }
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
private:
    QColor m_color = Qt::black;
    int m_lineWidth = 2;
};

class CircleDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override {
        if (points.size() < 2) return; // 至少需要两个点

        painter.setPen(QPen(m_color, m_lineWidth));
        QPointF center = points[0]; // 第一个点为圆心
        double radius = std::sqrt(std::pow(points[1].x() - center.x(), 2) +
                                  std::pow(points[1].y() - center.y(), 2));

        // 使用中点圆算法绘制圆
        drawCircle(painter, center, static_cast<int>(radius));
    }

    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }

private:
        void drawCircle(QPainter& painter, const QPointF& center, int radius)const;
};


class FlowchartNodeDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override;
    void setColor(const QColor& color) override;
    void setLineWidth(int width) override;
private:
    QColor m_color = Qt::black;
    int m_lineWidth = 2;
};

class EllipseDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override {
        if (points.size() < 2) return; // 至少需要两个点

        painter.setPen(QPen(m_color, m_lineWidth));
        QPointF center = points[0]; // 第一个点为中心
        QPointF sizePoint = points[1]; // 第二个点决定椭圆的大小

        double width = std::abs(sizePoint.x() - center.x()) * 2; // 宽度
        double height = std::abs(sizePoint.y() - center.y()) * 2; // 高度

        drawEllipse(painter, center, width / 2, height / 2);
    }

    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }

private:
    void drawEllipse(QPainter& painter, const QPointF& center, double a, double b) const;

    QColor m_color = Qt::black;
    int m_lineWidth = 2;
};


#endif // DRAW_STRATEGY_H