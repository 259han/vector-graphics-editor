#include "../../include/core/draw_strategy.h"

void LineDrawStrategy::draw(QPainter& painter, const std::vector<QPointF>& points) const{
    if (points.size() < 2) return; // 至少需要两个点

    // 使用 Bresenham 算法绘制直线
    QPointF start = points[0];
    QPointF end = points[1];

    int x1 = static_cast<int>(start.x());
    int y1 = static_cast<int>(start.y());
    int x2 = static_cast<int>(end.x());
    int y2 = static_cast<int>(end.y());

    int dx = x2 - x1;
    int dy = y2 - y1;
    int absDx = std::abs(dx);
    int absDy = std::abs(dy);
    int sx = (dx > 0) ? 1 : -1;
    int sy = (dy > 0) ? 1 : -1;

    if (absDx > absDy) {
        int err = absDx / 2;
        while (x1 != x2) {
            painter.drawPoint(x1, y1); // 绘制点
            err -= absDy;
            if (err < 0) {
                y1 += sy;
                err += absDx;
            }
            x1 += sx;
        }
    } else {
        int err = absDy / 2;
        while (y1 != y2) {
            painter.drawPoint(x1, y1); // 绘制点
            err -= absDx;
            if (err < 0) {
                x1 += sx;
                err += absDy;
            }
            y1 += sy;
        }
    }
    painter.drawPoint(x2, y2); // 确保绘制终点
}

void FlowchartNodeDrawStrategy::draw(QPainter& painter, const std::vector<QPointF>& points) const {
    if (points.size() < 2) return;

    painter.setPen(QPen(m_color, m_lineWidth));
    
    // 假设第一个点是节点的中心，第二个点决定节点大小和形状
    QPointF center = points[0];
    QPointF sizePoint = points[1];
    
    double width = std::abs(sizePoint.x() - center.x()) * 2;
    double height = std::abs(sizePoint.y() - center.y()) * 2;
    
    // 绘制带圆角的矩形作为流程图节点
    painter.drawRoundedRect(
        center.x() - width/2, 
        center.y() - height/2, 
        width, 
        height, 
        10, 10  // 圆角半径
    );
}

void FlowchartNodeDrawStrategy::setColor(const QColor& color) {
    m_color = color;
}

void FlowchartNodeDrawStrategy::setLineWidth(int width) {
    m_lineWidth = width;
}
void EllipseDrawStrategy::drawEllipse(QPainter& painter, const QPointF& center, double a, double b) const {
        // 使用中点椭圆算法绘制椭圆
        int x = 0;
        int y = b;
        int a2 = a * a;
        int b2 = b * b;
        int err = 0;

        // 绘制椭圆的四个象限
        while (x * b2 <= y * a2) {
            painter.drawPoint(center.x() + x, center.y() + y);
            painter.drawPoint(center.x() - x, center.y() + y);
            painter.drawPoint(center.x() + x, center.y() - y);
            painter.drawPoint(center.x() - x, center.y() - y);
            x++;
            err += 2 * b2 * x + b2;
            if (2 * a2 * y <= err) {
                y--;
                err -= 2 * a2 * y;
            }
        }

        x = a;
        y = 0;
        err = 0;

        // 绘制椭圆的四个象限
        while (y * a2 <= x * b2) {
            painter.drawPoint(center.x() + x, center.y() + y);
            painter.drawPoint(center.x() - x, center.y() + y);
            painter.drawPoint(center.x() + x, center.y() - y);
            painter.drawPoint(center.x() - x, center.y() - y);
            y++;
            err += 2 * a2 * y + a2;
            if (2 * b2 * x <= err) {
                x--;
                err -= 2 * b2 * x;
            }
        }
    }
void CircleDrawStrategy::drawCircle(QPainter& painter, const QPointF& center, int radius)const{
        int x = radius;
        int y = 0;
        int err = 0;

        while (x >= y) {
            // 绘制八个对称点
            painter.drawPoint(center.x() + x, center.y() + y);
            painter.drawPoint(center.x() + y, center.y() + x);
            painter.drawPoint(center.x() - y, center.y() + x);
            painter.drawPoint(center.x() - x, center.y() + y);
            painter.drawPoint(center.x() - x, center.y() - y);
            painter.drawPoint(center.x() - y, center.y() - x);
            painter.drawPoint(center.x() + y, center.y() - x);
            painter.drawPoint(center.x() + x, center.y() - y);

            y++;
            err += 1 + 2 * y;
            if (2 * (err - x) + 1 > 0) {
                x--;
                err += 1 - 2 * x;
            }
        }
}
void BezierDrawStrategy::draw(QPainter& painter, const std::vector<QPointF>& points) const {
    if (points.size() < 2) return; // At least two points are needed

    painter.setPen(QPen(m_color, m_lineWidth));
    drawBezierCurve(painter, points);
}

// draw_strategy.cpp
void BezierDrawStrategy::drawBezierCurve(QPainter& painter, const std::vector<QPointF>& points) const {
    if (points.size() < 2) return;

    const int steps = 200;
    QPointF prevPoint = points[0];
    
    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        std::vector<QPointF> temp = points;
        
        // 德卡斯特里奥算法
        for (int k = 1; k < temp.size(); ++k) {
            for (int j = 0; j < temp.size() - k; ++j) {
                temp[j] = (1 - t) * temp[j] + t * temp[j + 1];
            }
        }
        
        QPointF currentPoint = temp[0];
        painter.drawLine(prevPoint, currentPoint); // 绘制连续线段
        prevPoint = currentPoint;
    }
}

int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
} 