#include "bezier_graphic_item.h"
#include <QPainter>
#include <cmath>

BezierGraphicItem::BezierGraphicItem(const std::vector<QPointF>& controlPoints)
    : m_controlPoints(controlPoints)
{
    // 确保至少有两个控制点
    if (m_controlPoints.size() < 2) {
        m_controlPoints = {QPointF(0, 0), QPointF(100, 0)};
    }
    
    // 设置绘制策略
    setDrawStrategy(std::make_shared<BezierDrawStrategy>());
    
    // 更新几何信息
    updateGeometry();
}

QRectF BezierGraphicItem::boundingRect() const
{
    // 计算包含所有控制点的最小矩形
    if (m_controlPoints.empty()) {
        return QRectF();
    }
    
    double minX = m_controlPoints[0].x();
    double minY = m_controlPoints[0].y();
    double maxX = minX;
    double maxY = minY;
    
    for (const QPointF& point : m_controlPoints) {
        minX = std::min(minX, point.x());
        minY = std::min(minY, point.y());
        maxX = std::max(maxX, point.x());
        maxY = std::max(maxY, point.y());
    }
    
    // 添加一些边距
    const double margin = 5.0;
    return QRectF(minX - margin, minY - margin, 
                 (maxX - minX) + 2 * margin, (maxY - minY) + 2 * margin);
}

std::vector<QPointF> BezierGraphicItem::getDrawPoints() const
{
    return m_controlPoints;
}

void BezierGraphicItem::setControlPoints(const std::vector<QPointF>& controlPoints)
{
    // 确保至少有两个控制点
    if (controlPoints.size() < 2) {
        return;
    }
    
    m_controlPoints = controlPoints;
    updateGeometry();
    update(); // 重绘图形项
}

void BezierGraphicItem::addControlPoint(const QPointF& point)
{
    m_controlPoints.push_back(point);
    updateGeometry();
    update(); // 重绘图形项
}

void BezierGraphicItem::clearControlPoints()
{
    m_controlPoints.clear();
    m_controlPoints = {QPointF(0, 0), QPointF(100, 0)}; // 默认两点
    updateGeometry();
    update(); // 重绘图形项
}

QPointF BezierGraphicItem::getControlPoint(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size())) {
        return m_controlPoints[index];
    }
    return QPointF();
}

void BezierGraphicItem::setControlPoint(int index, const QPointF& point)
{
    if (index >= 0 && index < static_cast<int>(m_controlPoints.size())) {
        m_controlPoints[index] = point;
        updateGeometry();
        update(); // 重绘图形项
    }
}

void BezierGraphicItem::updateGeometry()
{
    // 计算并设置图形项位置（使用第一个控制点作为原点）
    if (!m_controlPoints.empty()) {
        // 将所有点相对于第一个点进行平移
        QPointF origin = m_controlPoints[0];
        setPos(origin);
        
        // 更新控制点为相对于图形项原点的坐标
        for (int i = 0; i < static_cast<int>(m_controlPoints.size()); ++i) {
            m_controlPoints[i] -= origin;
        }
    }
    
    // 更新包围矩形
    prepareGeometryChange();
}
