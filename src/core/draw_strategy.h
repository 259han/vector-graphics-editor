#ifndef DRAW_STRATEGY_H
#define DRAW_STRATEGY_H

#include <QPainter>
#include <QPointF>
#include <vector>
#include <QPainterPath>

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
};

class RectangleDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override {
        if (points.size() < 2) return;
        
        // 设置画笔
        painter.setPen(QPen(m_color, m_lineWidth));
        
        // 设置画刷为透明（无填充）
        QBrush oldBrush = painter.brush();
        painter.setBrush(Qt::NoBrush);
        
        // 创建矩形
        QRectF rect;
        if (points.size() >= 2) {
            rect = QRectF(points[0], points[1]);
        } else {
            // 安全保障
            return;
        }
        
        // 绘制矩形 - 确保用normalized()避免负宽度/高度
        painter.drawRect(rect.normalized());
        
        // 恢复原来的画刷
        painter.setBrush(oldBrush);
    }
    
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
};

class CircleDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override {
        if (points.size() < 2) return; // 至少需要两个点

        painter.setPen(QPen(m_color, m_lineWidth));
        QPointF center = points[0]; // 第一个点为圆心
        
        // 计算半径为两点之间的距离
        double radius = std::sqrt(std::pow(points[1].x() - center.x(), 2) +
                                std::pow(points[1].y() - center.y(), 2));
                                
        // 确保半径非负
        radius = std::max(0.0, radius);

        // 使用Qt的drawEllipse直接绘制圆形
        painter.drawEllipse(center, radius, radius);
    }

    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }

private:
    void drawCircle(QPainter& painter, const QPointF& center, int radius) const;
};

class EllipseDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override {
        if (points.size() < 2) return; // 至少需要两个点

        painter.setPen(QPen(m_color, m_lineWidth));
        
        // 设置无填充
        QBrush oldBrush = painter.brush();
        painter.setBrush(Qt::NoBrush);
        
        QPointF center = points[0]; // 第一个点为中心
        QPointF sizePoint = points[1]; // 第二个点确定半径大小
        
        // 计算半径 - 使用点之间的距离而不是直接使用坐标值
        double radiusX = std::abs(sizePoint.x() - center.x());
        double radiusY = std::abs(sizePoint.y() - center.y());
        
        // 检查半径是否有效，防止无限循环
        if (radiusX <= 0 || radiusY <= 0) {
            // 如果半径无效，绘制一个点
            painter.drawPoint(center);
            painter.setBrush(oldBrush);
            return;
        }
        
        // 对于小椭圆，直接使用Qt的drawEllipse
        if (radiusX < 10 || radiusY < 10) {
            painter.drawEllipse(center, radiusX, radiusY);
            painter.setBrush(oldBrush);
            return;
        }
        
        // 使用中点椭圆算法绘制椭圆
        drawEllipse(painter, center, radiusX, radiusY);
        
        // 恢复画刷
        painter.setBrush(oldBrush);
    }

    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }

private:
    void drawEllipse(QPainter& painter, const QPointF& center, double a, double b) const;
};

class BezierDrawStrategy : public DrawStrategy {
public:
    void draw(QPainter& painter, const std::vector<QPointF>& points) const override;
    void setColor(const QColor& color) override { m_color = color; }
    void setLineWidth(int width) override { m_lineWidth = width; }
    
private:
    // 计算n阶Bezier曲线上的点
    QPointF calculateBezierPoint(const std::vector<QPointF>& controlPoints, double t) const;
    // 计算二项式系数C(n,k)
    double binomialCoefficient(int n, int k) const;
};

#endif // DRAW_STRATEGY_H