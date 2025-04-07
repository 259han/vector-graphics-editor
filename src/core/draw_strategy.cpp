#include "draw_strategy.h"
#include <cmath>

void LineDrawStrategy::draw(QPainter* painter, const std::vector<QPointF>& points) {
    if (points.size() < 2) {
        return;
    }
    
    // 保存原始画笔
    QPen originalPen = painter->pen();
    
    // 设置自定义画笔
    QPen pen = originalPen;
    pen.setColor(m_color);
    pen.setWidthF(m_lineWidth);
    painter->setPen(pen);
    
    // 绘制直线
    painter->drawLine(points[0], points[1]);
    
    // 恢复原始画笔
    painter->setPen(originalPen);
}

void RectangleDrawStrategy::draw(QPainter* painter, const std::vector<QPointF>& points) {
    if (points.size() < 2) {
        return;
    }
    
    // 保存原始画笔和画刷
    QPen originalPen = painter->pen();
    QBrush originalBrush = painter->brush();
    
    // 设置自定义画笔
    QPen pen = originalPen;
    pen.setColor(m_color);
    pen.setWidthF(m_lineWidth);
    painter->setPen(pen);
    
    // 计算矩形区域
    QRectF rect(points[0], points[1]);
    
    // 绘制矩形
    painter->drawRect(rect);
    
    // 恢复原始画笔和画刷
    painter->setPen(originalPen);
    painter->setBrush(originalBrush);
}

void CircleDrawStrategy::draw(QPainter* painter, const std::vector<QPointF>& points) {
    if (points.size() < 2) {
        return;
    }
    
    // 保存原始画笔
    QPen originalPen = painter->pen();
    
    // 设置自定义画笔
    QPen pen = originalPen;
    pen.setColor(m_color);
    pen.setWidthF(m_lineWidth);
    painter->setPen(pen);
    
    // 对于圆形，第一个点是中心，第二个点用来确定半径
    QPointF center = points[0];
    QPointF radiusPoint = points[1];
    
    // 计算半径
    double radius = QLineF(center, radiusPoint).length();
    
    // 绘制圆
    painter->drawEllipse(center, radius, radius);
    
    // 恢复原始画笔
    painter->setPen(originalPen);
}

void EllipseDrawStrategy::draw(QPainter* painter, const std::vector<QPointF>& points) {
    if (points.size() < 2) {
        return;
    }
    
    // 保存原始画笔和画刷
    QPen originalPen = painter->pen();
    QBrush originalBrush = painter->brush();
    
    // 设置自定义画笔
    QPen pen = originalPen;
    pen.setColor(m_color);
    pen.setWidthF(m_lineWidth);
    painter->setPen(pen);
    
    // 从左上角和右下角点创建矩形
    QRectF rect(points[0], points[1]);
    
    // 绘制椭圆
    painter->drawEllipse(rect);
    
    // 恢复原始画笔和画刷
    painter->setPen(originalPen);
    painter->setBrush(originalBrush);
}

void BezierDrawStrategy::draw(QPainter* painter, const std::vector<QPointF>& points) {
    if (points.size() < 2) {
        return;
    }
    
    // 保存原始画笔
    QPen originalPen = painter->pen();
    
    // 设置自定义画笔
    QPen pen = originalPen;
    pen.setColor(m_color);
    pen.setWidthF(m_lineWidth);
    painter->setPen(pen);

    // 贝塞尔曲线的计算和绘制
    if (points.size() == 2) {
        // 只有两个点时，退化为直线
        painter->drawLine(points[0], points[1]);
    } else {
        // 创建路径
        QPainterPath path;
        path.moveTo(points[0]);

        // 计算三阶贝塞尔曲线（每四个点一组，起点、两个控制点、终点）
        for (size_t i = 0; i + 3 < points.size(); i += 3) {
            path.cubicTo(points[i+1], points[i+2], points[i+3]);
        }

        // 绘制路径
        painter->drawPath(path);
        
        // 移除调试用的控制点绘制，避免额外绘制开销
        // 以下代码在发布版中应该被注释掉
        /*
        painter->save();
        painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
        for (size_t i = 0; i + 1 < points.size(); i++) {
            painter->drawLine(points[i], points[i+1]);
        }
        painter->restore();
        */
    }
    
    // 恢复原始画笔
    painter->setPen(originalPen);
}

QPointF BezierDrawStrategy::calculateBezierPoint(const std::vector<QPointF>& controlPoints, double t) const {
    int n = controlPoints.size() - 1; // 曲线阶数
    double x = 0.0, y = 0.0;
    
    for (int i = 0; i <= n; i++) {
        double coefficient = binomialCoefficient(n, i) * pow(t, i) * pow(1 - t, n - i);
        x += coefficient * controlPoints[i].x();
        y += coefficient * controlPoints[i].y();
    }
    
    return QPointF(x, y);
}

double BezierDrawStrategy::binomialCoefficient(int n, int k) const {
    if (k < 0 || k > n) return 0.0;
    if (k == 0 || k == n) return 1.0;
    
    double result = 1.0;
    for (int i = 1; i <= k; i++) {
        result *= (n - (k - i));
        result /= i;
    }
    
    return result;
}