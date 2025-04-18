#include "draw_strategy.h"
#include <cmath>
#include <QThread>

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
    
    // 从提供的点创建矩形
    // 确保从左上角和右下角点创建标准化矩形
    QRectF rect(points[0], points[1]);
    
    // 绘制椭圆
    painter->drawEllipse(rect);
    
    // 恢复原始画笔和画刷
    painter->setPen(originalPen);
    painter->setBrush(originalBrush);
}

void BezierDrawStrategy::draw(QPainter* painter, const std::vector<QPointF>& points) {
    if (points.size() < 2) return;

    // 计算控制点折线总长度
    double totalPolylineLength = 0.0;
    for (size_t i = 1; i < points.size(); ++i) {
        totalPolylineLength += QLineF(points[i-1], points[i]).length();
    }

    // 动态步长计算
    const double densityFactor = 5.0;  // 每单位长度5个点
    const int minSteps = 20;           // 最小步数
    const int maxSteps = 500;          // 最大步数
    int numSteps = static_cast<int>(totalPolylineLength * densityFactor);
    numSteps = std::clamp(numSteps, minSteps, maxSteps);    //clamp限制范围小于min，大于max改为min，max
    
    // 保存原始画笔
    QPen originalPen = painter->pen();
    
    // 设置自定义画笔
    QPen pen = originalPen;
    pen.setColor(m_color);
    pen.setWidthF(m_lineWidth);
    painter->setPen(pen);

    // 贝塞尔曲线绘制逻辑
    QPainterPath path;
    if (points.size() == 2) {
        // 两个点退化为直线
        path.moveTo(points[0]);
        path.lineTo(points[1]);
    } else {
        // 伯恩斯坦公式
        path.moveTo(points[0]);
        for (int step = 1; step <= numSteps; ++step) {
            double t = static_cast<double>(step) / numSteps;//t为插值参数
            QPointF p = calculateBezierPoint(points, t);
            path.lineTo(p);
        }
    }
    
    // 绘制路径
    painter->drawPath(path);
    
    // 恢复原始画笔
    painter->setPen(originalPen);
}

//贝塞尔曲线点计算(递推)
QPointF BezierDrawStrategy::calculateBezierPoint(const std::vector<QPointF>& controlPoints, double t) const {
    std::vector<QPointF> tempPoints = controlPoints;
    int n = tempPoints.size();
    
    for (int k = 1; k < n; ++k) {
        for (int i = 0; i < n - k; ++i) {
            tempPoints[i] = (1 - t) * tempPoints[i] + t * tempPoints[i + 1];
        }
    }
    return tempPoints[0];
}
