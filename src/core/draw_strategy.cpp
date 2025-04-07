#include "draw_strategy.h"
#include <cmath>

void LineDrawStrategy::draw(QPainter& painter, const std::vector<QPointF>& points) const{
    if (points.size() < 2) return; // 至少需要两个点

    // 设置画笔
    painter.setPen(QPen(m_color, m_lineWidth));
    
    // 获取起点和终点
    QPointF start = points[0];
    QPointF end = points[1];
    
    // 确保起点和终点有效
    if (!start.isNull() && !end.isNull()) {
        // 直接使用Qt的drawLine绘制直线，更可靠且高效
        painter.drawLine(start, end);
    }
}

void EllipseDrawStrategy::drawEllipse(QPainter& painter, const QPointF& center, double a, double b) const {
    // 使用安全的椭圆绘制算法，替代中点椭圆算法以防止可能的问题
    // 采用参数方程 x = a*cos(t), y = b*sin(t) 绘制
    const int steps = 72; // 将圆周分为72份，每份5度
    
    // 检查半径是否有效，防止无限循环
    if (a <= 0 || b <= 0) {
        painter.drawPoint(center);
        return;
    }
    
    QPointF prevPoint;
    bool firstPoint = true;
    
    for (int i = 0; i <= steps; ++i) {
        double angle = 2.0 * M_PI * i / steps;
        double x = center.x() + a * std::cos(angle);
        double y = center.y() + b * std::sin(angle);
        QPointF currentPoint(x, y);
        
        if (firstPoint) {
            prevPoint = currentPoint;
            firstPoint = false;
            continue;
        }
        
        // 绘制连接线段
        painter.drawLine(prevPoint, currentPoint);
        prevPoint = currentPoint;
    }
}

void CircleDrawStrategy::drawCircle(QPainter& painter, const QPointF& center, int radius) const {
    // 检查半径是否有效
    if (radius <= 0) {
        painter.drawPoint(center);
        return;
    }
    
    // 使用Qt的drawEllipse直接绘制圆形，效率更高
    painter.drawEllipse(center, radius, radius);
    
    // 备用方法：如果需要使用手动绘制圆的算法，可以取消下面的注释
    /*
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
    */
}

void BezierDrawStrategy::draw(QPainter& painter, const std::vector<QPointF>& points) const {
    if (points.size() < 2) return; // 至少需要两个点
    
    // 设置画笔
    painter.setPen(QPen(m_color, m_lineWidth));
    
    // 使用QPainterPath绘制Bezier曲线
    QPainterPath path;
    path.moveTo(points[0]);
    
    if (points.size() == 2) {
        // 线性Bezier（直线）
        path.lineTo(points[1]);
    } else if (points.size() == 3) {
        // 二次Bezier曲线
        path.quadTo(points[1], points[2]);
    } else if (points.size() == 4) {
        // 三次Bezier曲线
        path.cubicTo(points[1], points[2], points[3]);
    } else {
        // n阶Bezier曲线，需要手动计算
        const int STEPS = 100; // 曲线平滑度
        
        for (int i = 1; i <= STEPS; i++) {
            double t = static_cast<double>(i) / STEPS;
            QPointF pt = calculateBezierPoint(points, t);
            path.lineTo(pt);
        }
    }
    
    painter.drawPath(path);
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