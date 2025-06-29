#include "clip_algorithms.h"
#include "../utils/logger.h"
#include <QDebug>
#include <QPolygonF>
#include <cmath>
#include <algorithm>
#include <vector>
#include <QPointF>
#include <QPainterPath>
#include <QImage>
#include <QPainter>
#include <QBitmap>
#include <QRegion>
#include <map>
#include <set>

namespace ClipAlgorithms {

// 内部使用的函数
namespace Internal {

// Cohen-Sutherland算法的区域编码
const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

// 线段表示
struct Edge {
    QPointF start;
    QPointF end;
};

// 交点信息
struct Intersection {
    QPointF point;
    size_t subjectEdgeIndex;
    size_t clipEdgeIndex;
};

// 顶点类型
struct VertexType {
    QPointF point;
    bool isIntersection;
    size_t edgeIndex;
    double t;  // 参数化位置
    bool isEntry;  // 是进入点还是退出点
    size_t correspondingIndex = 0;  // 对应另一个多边形中的索引
};

// 计算点相对于裁剪矩形的区域编码
int computeOutCode(const QPointF& p, const QRectF& clipRect) {
    int code = INSIDE;
    
    if (p.x() < clipRect.left())
        code |= LEFT;
    else if (p.x() > clipRect.right())
        code |= RIGHT;
    
    if (p.y() < clipRect.top())
        code |= TOP;
    else if (p.y() > clipRect.bottom())
        code |= BOTTOM;
    
    return code;
}

// 判断点是否在裁剪边缘的内侧
bool isInside(const QPointF& p, ClipEdge edge, const QRectF& clipRect) {
    switch (edge) {
        case ClipEdge::Left:
            return p.x() >= clipRect.left();
        case ClipEdge::Right:
            return p.x() <= clipRect.right();
        case ClipEdge::Bottom:
            return p.y() <= clipRect.bottom();
        case ClipEdge::Top:
            return p.y() >= clipRect.top();
    }
    return false;
}

// 计算两点与裁剪边缘的交点
QPointF computeIntersection(const QPointF& p1, const QPointF& p2, ClipEdge edge, const QRectF& clipRect) {
    const double EPSILON = 1e-8;
    
    switch (edge) {
        case ClipEdge::Left: {
            double x = clipRect.left();
            double t = (x - p1.x()) / ((p2.x() - p1.x()) < EPSILON ? EPSILON : (p2.x() - p1.x()));
            double y = p1.y() + t * (p2.y() - p1.y());
            return QPointF(x, y);
        }
        case ClipEdge::Right: {
            double x = clipRect.right();
            double t = (x - p1.x()) / ((p2.x() - p1.x()) < EPSILON ? EPSILON : (p2.x() - p1.x()));
            double y = p1.y() + t * (p2.y() - p1.y());
            return QPointF(x, y);
        }
        case ClipEdge::Bottom: {
            double y = clipRect.bottom();
            double t = (y - p1.y()) / ((p2.y() - p1.y()) < EPSILON ? EPSILON : (p2.y() - p1.y()));
            double x = p1.x() + t * (p2.x() - p1.x());
            return QPointF(x, y);
        }
        case ClipEdge::Top: {
            double y = clipRect.top();
            double t = (y - p1.y()) / ((p2.y() - p1.y()) < EPSILON ? EPSILON : (p2.y() - p1.y()));
            double x = p1.x() + t * (p2.x() - p1.x());
            return QPointF(x, y);
        }
    }
    return QPointF();
}

// 对多边形在单条边上进行裁剪
std::vector<QPointF> clipPolygonToEdge(const std::vector<QPointF>& vertices, ClipEdge edge, const QRectF& clipRect) {
    std::vector<QPointF> outputList;
    size_t size = vertices.size();
    
    if (size == 0) {
        return outputList;
    }
    
    QPointF s = vertices[size - 1];
    bool sInside = isInside(s, edge, clipRect);
    
    for (size_t i = 0; i < size; i++) {
        QPointF e = vertices[i];
        bool eInside = isInside(e, edge, clipRect);
        
        if (eInside) {
            if (!sInside) {
                outputList.push_back(computeIntersection(s, e, edge, clipRect));
            }
            outputList.push_back(e);
        } else if (sInside) {
            outputList.push_back(computeIntersection(s, e, edge, clipRect));
        }
        
        s = e;
        sInside = eInside;
    }
    
    return outputList;
}

// 计算两线段的交点
bool lineIntersection(const QPointF& p1, const QPointF& p2, 
                     const QPointF& p3, const QPointF& p4, 
                     QPointF& intersection) {
    double denominator = (p4.y() - p3.y()) * (p2.x() - p1.x()) - 
                        (p4.x() - p3.x()) * (p2.y() - p1.y());
    
    if (qFuzzyIsNull(denominator)) {
        return false;
    }
    
    double ua = ((p4.x() - p3.x()) * (p1.y() - p3.y()) - 
                (p4.y() - p3.y()) * (p1.x() - p3.x())) / denominator;
    double ub = ((p2.x() - p1.x()) * (p1.y() - p3.y()) - 
                (p2.y() - p1.y()) * (p1.x() - p3.x())) / denominator;
    
    if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0) {
        intersection.setX(p1.x() + ua * (p2.x() - p1.x()));
        intersection.setY(p1.y() + ua * (p2.y() - p1.y()));
        return true;
    }
    
    return false;
}

// 判断点是否在多边形内部 (使用射线法)
bool pointInPolygon(const QPointF& point, const std::vector<QPointF>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }
    
    bool inside = false;
    size_t i, j;
    for (i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        if (((polygon[i].y() > point.y()) != (polygon[j].y() > point.y())) &&
            (point.x() < (polygon[j].x() - polygon[i].x()) * (point.y() - polygon[i].y()) / 
                         (polygon[j].y() - polygon[i].y()) + polygon[i].x())) {
            inside = !inside;
        }
    }
    
    return inside;
}

// 点是否在线段上
bool isPointOnLineSegment(const QPointF& p, const QPointF& lineStart, const QPointF& lineEnd) {
    QLineF line(lineStart, lineEnd);
    double lineLength = line.length();
    
    if (lineLength < 0.001) {
        return QLineF(p, lineStart).length() < 0.001;
    }
    
    double t = ((p.x() - lineStart.x()) * (lineEnd.x() - lineStart.x()) + 
               (p.y() - lineStart.y()) * (lineEnd.y() - lineStart.y())) / 
              (lineLength * lineLength);
    
    if (t < 0 || t > 1) {
        return false;
    }
    
    QPointF projection = lineStart + t * (lineEnd - lineStart);
    return QLineF(p, projection).length() < 0.001;
}

// 计算点在线段上的参数位置
double parameterAlongLine(const QPointF& lineStart, const QPointF& lineEnd, const QPointF& point) {
    QPointF v = lineEnd - lineStart;
    double vLengthSquared = v.x()*v.x() + v.y()*v.y();
    
    if (vLengthSquared < 0.0001) {
        return 0.0; // 线段长度接近于0
    }
    
    QPointF w = point - lineStart;
    double proj = QPointF::dotProduct(w, v) / vLengthSquared;
    
    return proj;
}

// 检查点是否在路径边界附近
bool isPointNearBoundary(const QPointF& point, const QPainterPath& path, double tolerance) {
    QPainterPathStroker stroker;
    stroker.setWidth(tolerance * 2);
    return stroker.createStroke(path).contains(point);
}

// 验证裁剪结果
bool validateIntersection(const QPainterPath& result, const QPainterPath& clip) {
    //检查结果是否为空
    if (result.isEmpty()) {
        Logger::debug("validateIntersection: 结果路径为空");
        return false;
    }
    
    //检查边界框
    QRectF resultBounds = result.boundingRect();
    QRectF clipBounds = clip.boundingRect();
    if (!clipBounds.contains(resultBounds)) {
        Logger::debug("validateIntersection: 结果边界框超出裁剪路径边界");
        return false;
    }
    
    //检查面积
    double resultArea = resultBounds.width() * resultBounds.height();
    double clipArea = clipBounds.width() * clipBounds.height();
    if (resultArea > clipArea) {
        Logger::debug("validateIntersection: 结果面积大于裁剪路径面积");
        return false;
    }
    
    //检查顶点
    std::vector<QPointF> vertices = pathToPoints(result, 1.0); // 使用较大的flatness值只获取顶点
    const double tolerance = 0.001; // 容差值
    
    int validPoints = 0;
    for (const auto& vertex : vertices) {
        if (clip.contains(vertex) || isPointNearBoundary(vertex, clip, tolerance)) {
            validPoints++;
        }
    }
    
    // 允许95%的点
    double validRatio = static_cast<double>(validPoints) / vertices.size();
    bool isValid = validRatio > 0.95;
    
    if (!isValid) {
        Logger::debug(QString("validateIntersection: 有效点比例 %1 低于阈值 0.95").arg(validRatio));
    }
    
    return isValid;
}

} // namespace Internal

// 使用Sutherland-Hodgman算法裁剪多边形
std::vector<QPointF> sutherlandHodgmanClip(const std::vector<QPointF>& subjectPolygon, 
                                          const QRectF& clipRect) {
    Logger::debug(QString("SutherlandHodgman: 裁剪多边形，顶点数: %1")
                 .arg(subjectPolygon.size()));
    
    // 检查输入是否有效
    if (subjectPolygon.size() < 3) {
        Logger::warning("SutherlandHodgman: 输入多边形顶点数不足");
        return {};
    }
    
    // 按照边界顺序依次裁剪
    std::vector<QPointF> outputList = subjectPolygon;
    
    // 对每条裁剪边进行处理
    for (int i = 0; i < 4; i++) {
        ClipEdge edge = static_cast<ClipEdge>(i);
        outputList = Internal::clipPolygonToEdge(outputList, edge, clipRect);
        
        if (outputList.empty()) {
            Logger::debug(QString("SutherlandHodgman: 在边 %1 裁剪后，多边形为空").arg(i));
            return {};
        }
    }
    
    Logger::debug(QString("SutherlandHodgman: 裁剪完成，结果顶点数: %1")
                 .arg(outputList.size()));
    
    return outputList;
}

// 使用Cohen-Sutherland算法裁剪线段
bool cohenSutherlandClip(QPointF& p1, QPointF& p2, const QRectF& clipRect) {
    Logger::debug(QString("CohenSutherland: 裁剪线段 (%1,%2)-(%3,%4)")
                 .arg(p1.x()).arg(p1.y())
                 .arg(p2.x()).arg(p2.y()));
    
    // 计算初始点的区域码
    int code1 = Internal::computeOutCode(p1, clipRect);
    int code2 = Internal::computeOutCode(p2, clipRect);
    
    bool accept = false;
    
    while (true) {
        // 两点都在裁剪窗口内，接受线段
        if ((code1 | code2) == 0) {
            accept = true;
            break;
        }
        // 两点都在裁剪窗口外的同一侧，拒绝线段
        else if ((code1 & code2) != 0) {
            break;
        }
        // 线段跨越裁剪边界，计算交点
        else {
            int codeOut = code1 != 0 ? code1 : code2;
            QPointF intersection;
            
            // 根据区域码计算交点
            if (codeOut & Internal::TOP) {
                // 与上边界相交
                intersection.setX(p1.x() + (p2.x() - p1.x()) * (clipRect.top() - p1.y()) / (p2.y() - p1.y()));
                intersection.setY(clipRect.top());
            } else if (codeOut & Internal::BOTTOM) {
                // 与下边界相交
                intersection.setX(p1.x() + (p2.x() - p1.x()) * (clipRect.bottom() - p1.y()) / (p2.y() - p1.y()));
                intersection.setY(clipRect.bottom());
            } else if (codeOut & Internal::RIGHT) {
                // 与右边界相交
                intersection.setY(p1.y() + (p2.y() - p1.y()) * (clipRect.right() - p1.x()) / (p2.x() - p1.x()));
                intersection.setX(clipRect.right());
            } else if (codeOut & Internal::LEFT) {
                // 与左边界相交
                intersection.setY(p1.y() + (p2.y() - p1.y()) * (clipRect.left() - p1.x()) / (p2.x() - p1.x()));
                intersection.setX(clipRect.left());
            }
            
            // 替换线段的一个端点
            if (codeOut == code1) {
                p1 = intersection;
                code1 = Internal::computeOutCode(p1, clipRect);
            } else {
                p2 = intersection;
                code2 = Internal::computeOutCode(p2, clipRect);
            }
        }
    }
    
    Logger::debug(QString("CohenSutherland: 裁剪结果 %1，裁剪后线段 (%2,%3)-(%4,%5)")
                 .arg(accept ? "接受" : "拒绝")
                 .arg(p1.x()).arg(p1.y())
                 .arg(p2.x()).arg(p2.y()));
    
    return accept;
}

// 将路径转换为点集合
std::vector<QPointF> pathToPoints(const QPainterPath& path, qreal flatness) {
    Logger::debug(QString("pathToPoints: 开始转换路径，元素数量: %1，平滑度: %2")
                 .arg(path.elementCount())
                 .arg(flatness));
    
    std::vector<QPointF> points;
    
    // 处理空路径
    if (path.isEmpty()) {
        Logger::warning("pathToPoints: 输入路径为空");
        return points;
    }
    
    // 使用路径的elementCount函数获取路径元素数量
    int elementCount = path.elementCount();
    
    // 如果路径非常简单（只有少量元素），直接使用元素点
    if (elementCount <= 4) {
        for (int i = 0; i < elementCount; ++i) {
            QPainterPath::Element element = path.elementAt(i);
            points.push_back(QPointF(element.x, element.y));
        }
        
        // 确保闭合
        if (points.size() > 2 && 
            (points.front().x() != points.back().x() || 
             points.front().y() != points.back().y())) {
            points.push_back(points.front());
        }
        
        Logger::debug(QString("pathToPoints: 简单路径处理完成，提取了 %1 个点")
                     .arg(points.size()));
        return points;
    }

    // 使用QPolygonF获取路径的近似多边形表示
    QPolygonF polygon = path.toFillPolygon();
    
    // 如果多边形表示有效且点数足够，使用它
    if (!polygon.isEmpty() && polygon.size() > 2) {
        points.reserve(polygon.size());
        for (const QPointF& point : polygon) {
            points.push_back(point);
        }
        
        Logger::debug(QString("pathToPoints: 通过toFillPolygon提取了 %1 个点")
                     .arg(points.size()));
        return points;
    }
    
    // 如果前面的方法失败，手动提取路径点
    Logger::debug("pathToPoints: 回退到手动提取路径点");
    
    // 存储上一个点，用于检测曲线
    QPointF lastPoint;
    QPointF controlPoint1, controlPoint2;
    bool hasLastPoint = false;
    
    // 遍历路径元素
    for (int i = 0; i < elementCount; ++i) {
        QPainterPath::Element element = path.elementAt(i);
        
        switch (element.type) {
            case QPainterPath::MoveToElement:
                // 移动到新位置
                if (hasLastPoint && !points.empty() && 
                    (points.front().x() != lastPoint.x() || 
                     points.front().y() != lastPoint.y())) {
                    // 如果有上一个点且不等于第一个点，闭合前一个子路径
                    points.push_back(points.front());
                }
                
                lastPoint = QPointF(element.x, element.y);
                points.push_back(lastPoint);
                hasLastPoint = true;
                break;
                
            case QPainterPath::LineToElement:
                // 添加直线
                lastPoint = QPointF(element.x, element.y);
                points.push_back(lastPoint);
                hasLastPoint = true;
                break;
                
            case QPainterPath::CurveToElement:
                // 贝塞尔曲线的第一个控制点
                controlPoint1 = QPointF(element.x, element.y);
                break;
                
            case QPainterPath::CurveToDataElement:
                // 曲线的第二个控制点或终点
                if (i + 1 < elementCount && path.elementAt(i + 1).type == QPainterPath::CurveToDataElement) {
                    // 这是第二个控制点
                    controlPoint2 = QPointF(element.x, element.y);
                } else {
                    // 这是终点，生成曲线上的点
                    QPointF endPoint(element.x, element.y);
                    
                    // 计算曲线上的点
                    if (hasLastPoint) {
                        // 三次贝塞尔曲线采样
                        int numSegments = std::max(10, int(1.0 / flatness));
                        for (int j = 1; j <= numSegments; ++j) {
                            qreal t = j / qreal(numSegments);
                            qreal u = 1.0 - t;
                            qreal tt = t * t;
                            qreal uu = u * u;
                            qreal uuu = uu * u;
                            qreal ttt = tt * t;
                            
                            QPointF point(
                                uuu * lastPoint.x() + 3 * uu * t * controlPoint1.x() + 3 * u * tt * controlPoint2.x() + ttt * endPoint.x(),
                                uuu * lastPoint.y() + 3 * uu * t * controlPoint1.y() + 3 * u * tt * controlPoint2.y() + ttt * endPoint.y()
                            );
                            
                            points.push_back(point);
                        }
                        
                        lastPoint = endPoint;
                        hasLastPoint = true;
                    }
                }
                break;
        }
    }
    
    // 确保闭合
    if (points.size() > 2 && 
        (points.front().x() != points.back().x() || 
         points.front().y() != points.back().y())) {
        points.push_back(points.front());
    }
    
    // 对点集进行平滑处理 - 去除过于密集的点
    if (points.size() > 100) {
        Logger::debug("pathToPoints: 点数过多，进行平滑处理");
        
        std::vector<QPointF> smoothedPoints;
        smoothedPoints.reserve(points.size() / 2);
        
        smoothedPoints.push_back(points.front());
        
        for (size_t i = 1; i < points.size() - 1; ++i) {
            // 计算当前点到前一个保留点的距离
            QPointF diff = points[i] - smoothedPoints.back();
            qreal distance = QPointF::dotProduct(diff, diff);
            
            // 如果距离足够大，保留该点
            if (distance > flatness * flatness * 10) {
                smoothedPoints.push_back(points[i]);
            }
        }
        
        smoothedPoints.push_back(points.back());
        
        Logger::debug(QString("pathToPoints: 平滑后点数从 %1 减少到 %2")
                     .arg(points.size())
                     .arg(smoothedPoints.size()));
        
        return smoothedPoints;
    }
    
    Logger::debug(QString("pathToPoints: 手动提取完成，共提取 %1 个点")
                 .arg(points.size()));
    
    return points;
}

// 将点集合转换为路径
QPainterPath pointsToPath(const std::vector<QPointF>& points, bool closed) {
    QPainterPath path;
    
    if (points.empty()) {
        Logger::warning("pointsToPath: 输入点集为空");
        return path;
    }
    
    path.moveTo(points[0]);
    
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }
    
    if (closed && points.size() > 2 && 
        (points.front().x() != points.back().x() || 
         points.front().y() != points.back().y())) {
        path.closeSubpath();
    }
    
    return path;
}

// 判断路径是否接近矩形
bool isPathRectangular(const QPainterPath& path, qreal tolerance) {
    // 获取路径的边界矩形
    QRectF bounds = path.boundingRect();
    
    // 如果路径为空或太小，返回false
    if (path.isEmpty() || bounds.width() < 1 || bounds.height() < 1) {
        return false;
    }
    
    // 从路径中提取点
    std::vector<QPointF> points = pathToPoints(path);
    
    // 如果点太少，可能不是矩形
    if (points.size() < 4) {
        return false;
    }
    
    // 计算边界矩形的面积
    qreal rectArea = bounds.width() * bounds.height();
    
    // 计算多边形的面积
    qreal polyArea = 0;
    for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
        polyArea += (points[j].x() + points[i].x()) * (points[j].y() - points[i].y());
    }
    polyArea = qAbs(polyArea) / 2.0;
    
    // 计算面积比
    qreal areaRatio = qAbs(polyArea / rectArea - 1.0);
    
    // 如果面积比接近1，则认为路径是矩形
    return areaRatio < tolerance;
}

// 添加自定义的路径交集实现，模仿Qt的intersected方法
QPainterPath customIntersected(const QPainterPath& subject, const QPainterPath& clip) {
    Logger::debug("customIntersected: 开始计算路径交集");
    
    // 快速检查：如果路径为空，返回空路径
    if (subject.isEmpty() || clip.isEmpty()) {
        Logger::debug("customIntersected: 主体或裁剪路径为空，返回空路径");
        return QPainterPath();
    }
    
    // 转换为点集，用于裁剪算法
    double flatness = 0.1; // 较小的值能获得更高的精度
    std::vector<QPointF> subjectPoints = pathToPoints(subject, flatness);
    std::vector<QPointF> clipPoints = pathToPoints(clip, flatness);
    
    Logger::debug(QString("customIntersected: 主体路径提取点数 %1，裁剪路径提取点数 %2")
                 .arg(subjectPoints.size())
                 .arg(clipPoints.size()));
    
    // 检查点集是否有效
    if (subjectPoints.size() < 3 || clipPoints.size() < 3) {
        Logger::warning("customIntersected: 点集合不足，无法计算交集");
        return QPainterPath();
    }
    
    // 使用Weiler-Atherton裁剪算法计算两个多边形的交集
    std::vector<QPointF> resultPoints = weilerAthertonClip(subjectPoints, clipPoints);
    
    // 创建结果路径
    QPainterPath resultPath;
    if (resultPoints.size() >= 3) {
        // 将点集转换为路径
        resultPath = pointsToPath(resultPoints);
        
        // 使用新的验证方法检查结果
        if (!Internal::validateIntersection(resultPath, clip)) {
            Logger::debug("customIntersected: 验证失败，尝试使用栅格化方法");
            QPainterPath rasterPath = rasterizeIntersection(subject, clip);
            
            if (!rasterPath.isEmpty()) {
                resultPath = rasterPath;
                Logger::debug(QString("customIntersected: 使用栅格化方法创建的路径包含 %1 个元素")
                             .arg(resultPath.elementCount()));
            }
        }
    }
    
    return resultPath;
}

// Weiler-Atherton算法裁剪任意多边形
std::vector<QPointF> weilerAthertonClip(const std::vector<QPointF>& subjectPolygon, 
                                      const std::vector<QPointF>& clipPolygon) {
    Logger::debug(QString("WeilerAtherton: 裁剪多边形，主体顶点数: %1，裁剪顶点数: %2")
                 .arg(subjectPolygon.size())
                 .arg(clipPolygon.size()));
    
    if (subjectPolygon.size() < 3 || clipPolygon.size() < 3) {
        Logger::warning("WeilerAtherton: 输入多边形顶点数不足");
        return {};
    }
    
    // 构建边列表
    std::vector<Internal::Edge> subjectEdges;
    std::vector<Internal::Edge> clipEdges;
    
    for (size_t i = 0; i < subjectPolygon.size(); ++i) {
        size_t j = (i + 1) % subjectPolygon.size();
        subjectEdges.push_back({subjectPolygon[i], subjectPolygon[j]});
    }
    
    for (size_t i = 0; i < clipPolygon.size(); ++i) {
        size_t j = (i + 1) % clipPolygon.size();
        clipEdges.push_back({clipPolygon[i], clipPolygon[j]});
    }
    
    // 计算所有交点
    std::vector<Internal::Intersection> intersections;
    
    auto isPointNear = [](const QPointF& p1, const QPointF& p2, double epsilon = 0.001) -> bool {
        return QLineF(p1, p2).length() < epsilon;
    };
    
    for (size_t i = 0; i < subjectEdges.size(); ++i) {
        for (size_t j = 0; j < clipEdges.size(); ++j) {
            QPointF intersection;
            if (Internal::lineIntersection(subjectEdges[i].start, subjectEdges[i].end,
                                         clipEdges[j].start, clipEdges[j].end,
                                         intersection)) {
                if (Internal::isPointOnLineSegment(intersection, subjectEdges[i].start, subjectEdges[i].end) &&
                    Internal::isPointOnLineSegment(intersection, clipEdges[j].start, clipEdges[j].end)) {
                    
                    bool isDuplicate = false;
                    for (const auto& existingIntersection : intersections) {
                        if (isPointNear(existingIntersection.point, intersection)) {
                            isDuplicate = true;
                            break;
                        }
                    }
                    
                    if (!isDuplicate) {
                        intersections.push_back({intersection, i, j});
                    }
                }
            }
        }
    }
    
    Logger::debug(QString("WeilerAtherton: 计算出 %1 个交点").arg(intersections.size()));
    
    // 如果没有交点，检查包含关系
    if (intersections.empty()) {
        bool subjectInsideClip = true;
        for (const QPointF& point : subjectPolygon) {
            if (!Internal::pointInPolygon(point, clipPolygon)) {
                subjectInsideClip = false;
                break;
            }
        }
        
        if (subjectInsideClip) {
            Logger::debug("WeilerAtherton: 主体多边形完全在裁剪多边形内部，返回主体多边形");
            return subjectPolygon;
        }
        
        bool clipInsideSubject = true;
        for (const QPointF& point : clipPolygon) {
            if (!Internal::pointInPolygon(point, subjectPolygon)) {
                clipInsideSubject = false;
                break;
            }
        }
        
        if (clipInsideSubject) {
            Logger::debug("WeilerAtherton: 裁剪多边形完全在主体多边形内部，返回裁剪多边形");
            return clipPolygon;
        }
        
        Logger::debug("WeilerAtherton: 多边形无交点且不互相包含，无交集");
        return {};
    }
    
    // 使用交点构建结果
    std::vector<QPointF> result;
    
    // 添加位于裁剪多边形内的主体多边形点
    for (const QPointF& point : subjectPolygon) {
        if (Internal::pointInPolygon(point, clipPolygon)) {
            result.push_back(point);
        }
    }
    
    // 添加位于主体多边形内的裁剪多边形点
    for (const QPointF& point : clipPolygon) {
        if (Internal::pointInPolygon(point, subjectPolygon)) {
            result.push_back(point);
        }
    }
    
    // 添加所有交点
    for (const auto& intersection : intersections) {
        result.push_back(intersection.point);
    }
    
    // 如果点数足够，按照凸包方式排序
    if (result.size() >= 3) {
        // 计算质心
        QPointF centroid(0, 0);
        for (const QPointF& point : result) {
            centroid += point;
        }
        centroid /= result.size();
        
        // 按照点到质心的角度排序
        std::sort(result.begin(), result.end(), 
                 [&centroid](const QPointF& a, const QPointF& b) {
                     return std::atan2(a.y() - centroid.y(), a.x() - centroid.x()) < 
                            std::atan2(b.y() - centroid.y(), b.x() - centroid.x());
                 });
        
        // 移除太接近的点
        auto it = std::unique(result.begin(), result.end(), 
                             [](const QPointF& a, const QPointF& b) {
                                 return QLineF(a, b).length() < 0.001;
                             });
        result.erase(it, result.end());
        
        // 确保闭合
        if (result.size() > 0 && 
            QLineF(result.front(), result.back()).length() > 0.001) {
            result.push_back(result.front());
        }
        
        Logger::debug(QString("WeilerAtherton: 裁剪完成，结果顶点数: %1").arg(result.size()));
        return result;
    }
    
    Logger::warning("WeilerAtherton: 无法生成有效结果");
    return {};
}

// 当Weiler-Atherton算法无法找到有效结果时，使用栅格化方法作为备选
QPainterPath rasterizeIntersection(const QPainterPath& subject, const QPainterPath& clip) {
    Logger::debug("rasterizeIntersection: 使用栅格化方法计算路径交集");
    
    // 获取边界矩形
    QRectF subjectBounds = subject.boundingRect();
    QRectF clipBounds = clip.boundingRect();
    QRectF combinedBounds = subjectBounds.united(clipBounds);
    
    // 使用足够高的分辨率
    int resolution = 1000;
    QImage subjectImage(resolution, resolution, QImage::Format_ARGB32);
    QImage clipImage(resolution, resolution, QImage::Format_ARGB32);
    QImage resultImage(resolution, resolution, QImage::Format_ARGB32);
    QImage outlineImage(resolution, resolution, QImage::Format_ARGB32);
    
    subjectImage.fill(Qt::transparent);
    clipImage.fill(Qt::transparent);
    resultImage.fill(Qt::transparent);
    outlineImage.fill(Qt::transparent);
    
    // 计算变换矩阵，将路径映射到图像空间
    QTransform transform;
    transform.scale(resolution / combinedBounds.width(), 
                   resolution / combinedBounds.height());
    transform.translate(-combinedBounds.left(), -combinedBounds.top());
    
    // 绘制主体路径（使用填充用于计算交集）
    QPainter subjectPainter(&subjectImage);
    subjectPainter.setRenderHint(QPainter::Antialiasing);
    subjectPainter.setTransform(transform);
    subjectPainter.setPen(Qt::NoPen);
    subjectPainter.setBrush(Qt::white);
    subjectPainter.drawPath(subject);
    subjectPainter.end();
    
    // 绘制裁剪路径（使用填充用于计算交集）
    QPainter clipPainter(&clipImage);
    clipPainter.setRenderHint(QPainter::Antialiasing);
    clipPainter.setTransform(transform);
    clipPainter.setPen(Qt::NoPen);
    clipPainter.setBrush(Qt::white);
    clipPainter.drawPath(clip);
    clipPainter.end();
    
    // 计算交集图像
    for (int y = 0; y < resolution; y++) {
        for (int x = 0; x < resolution; x++) {
            if (qAlpha(subjectImage.pixel(x, y)) > 0 && qAlpha(clipImage.pixel(x, y)) > 0) {
                resultImage.setPixel(x, y, qRgba(255, 255, 255, 255));
            }
        }
    }
    
    // 检查结果是否为空
    bool hasIntersection = false;
    for (int y = 0; y < resolution && !hasIntersection; y++) {
        for (int x = 0; x < resolution; x++) {
            if (qAlpha(resultImage.pixel(x, y)) > 0) {
                hasIntersection = true;
                break;
            }
        }
    }
    
    if (!hasIntersection) {
        Logger::debug("rasterizeIntersection: 栅格化方法未找到交集");
        return QPainterPath();
    }
    
    // 创建轮廓图像，用于边界检测
    QPainter outlinePainter(&outlineImage);
    outlinePainter.drawImage(0, 0, resultImage);
    outlinePainter.end();
    
    // 找出所有边界像素
    QImage edgeImage(resolution, resolution, QImage::Format_ARGB32);
    edgeImage.fill(Qt::transparent);
    
    // 找出所有边界像素
    for (int y = 1; y < resolution-1; y++) {
        for (int x = 1; x < resolution-1; x++) {
            // 如果是边界像素（当前像素为白色且至少有一个相邻像素为透明）
            if (qAlpha(resultImage.pixel(x, y)) > 0) {
                bool isBoundary = false;
                
                // 检查4个相邻像素（上下左右）
                if (qAlpha(resultImage.pixel(x-1, y)) == 0 || 
                    qAlpha(resultImage.pixel(x+1, y)) == 0 || 
                    qAlpha(resultImage.pixel(x, y-1)) == 0 || 
                    qAlpha(resultImage.pixel(x, y+1)) == 0) {
                    isBoundary = true;
                }
                
                if (isBoundary) {
                    edgeImage.setPixel(x, y, qRgba(255, 255, 255, 255));
                }
            }
        }
    }
    
    // 实现改进的边界跟踪算法
    QPainterPath resultPath;
    
    // 查找起始边界点
    QPoint startPoint(-1, -1);
    for (int y = 0; y < resolution && startPoint.x() == -1; y++) {
        for (int x = 0; x < resolution; x++) {
            if (qAlpha(edgeImage.pixel(x, y)) > 0) {
                startPoint = QPoint(x, y);
                break;
            }
        }
    }
    
    if (startPoint.x() == -1) {
        Logger::warning("rasterizeIntersection: 无法找到起始边界点");
        // 创建路径
        QBitmap bitmap = QBitmap::fromImage(resultImage.createAlphaMask());
        QRegion region(bitmap);
        resultPath.addRegion(region);
        
        // 应用反向变换
        QTransform inverseTransform = transform.inverted();
        resultPath = inverseTransform.map(resultPath);
        
        return resultPath;
    }
    
    // 实现Moore邻域边界跟踪算法（针对8连通边界）
    std::vector<QPoint> contourPixels;
    QPoint currentPoint = startPoint;
    QPoint prevPoint(-1, -1);  // 初始化为无效点
    
    // 定义8个方向的偏移（顺时针排列）：右、右下、下、左下、左、左上、上、右上
    const int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    const int dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    
    const int MAX_CONTOUR_SIZE = resolution * 4; // 设置最大轮廓大小防止无限循环
    int dirStart = 7; // 从右上方向开始搜索
    
    // 添加起始点
    contourPixels.push_back(currentPoint);
    
    do {
        bool found = false;
        int dir = (dirStart + 6) % 8; // 初始搜索方向为起始方向的反时针2个位置
        
        // 尝试8个方向
        for (int i = 0; i < 8; i++) {
            dir = (dir + 1) % 8; // 顺时针搜索下一方向
            int nx = currentPoint.x() + dx[dir];
            int ny = currentPoint.y() + dy[dir];
            
            // 检查是否越界
            if (nx < 0 || nx >= resolution || ny < 0 || ny >= resolution) {
                continue;
            }
            
            // 找到下一个边界点
            if (qAlpha(edgeImage.pixel(nx, ny)) > 0) {
                // 避免立即返回到前一个点（除非只有一个边界点）
                if (prevPoint.x() != -1 && nx == prevPoint.x() && ny == prevPoint.y() && contourPixels.size() > 1) {
                    continue;
                }
                
                prevPoint = currentPoint;
                currentPoint = QPoint(nx, ny);
                contourPixels.push_back(currentPoint);
                
                // 更新下次搜索的起始方向
                dirStart = dir;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // 如果无法找到下一个边界点，说明轮廓追踪结束
            break;
        }
        
        // 避免无限循环
        if (contourPixels.size() > MAX_CONTOUR_SIZE) {
            Logger::warning("rasterizeIntersection: 轮廓追踪达到最大大小限制，可能存在问题");
            break;
        }
        
    } while (!(currentPoint == startPoint) || contourPixels.size() <= 2);
    
    // 移除重复相邻点并转换为原始坐标系
    std::vector<QPointF> contourPoints;
    QPoint lastPoint(-1, -1);
    
    for (const QPoint& pixel : contourPixels) {
        if (pixel != lastPoint) {
            QPointF scenePoint = transform.inverted().map(QPointF(pixel));
            contourPoints.push_back(scenePoint);
            lastPoint = pixel;
        }
    }
    
    // 简化轮廓点集 - 使用Douglas-Peucker算法的简化实现
    if (contourPoints.size() > 100) {
        std::vector<QPointF> simplifiedContour;
        simplifiedContour.reserve(100);
        
        // 简化参数
        double epsilon = combinedBounds.width() * 0.002; // 根据图形大小调整精度
        
        // 递归函数用于Douglas-Peucker简化
        std::function<void(const std::vector<QPointF>&, int, int, double, std::vector<bool>&)> 
        douglasPeucker = [&](const std::vector<QPointF>& points, int start, int end, double eps, std::vector<bool>& keep) {
            double maxDist = 0;
            int maxIndex = start;
            
            // 找到距离线段最远的点
            QLineF line(points[start], points[end]);
            for (int i = start + 1; i < end; i++) {
                double dist = QLineF(points[i], line.p1()).length() * QLineF(points[i], line.p2()).length();
                if (dist > 0) {
                    dist = QLineF(points[i], line.p1()).length() * QLineF(points[i], line.p2()).length() / 
                          line.length();
                    if (dist > maxDist) {
                        maxDist = dist;
                        maxIndex = i;
                    }
                }
            }
            
            // 如果最大距离大于阈值，递归处理两个子区间
            if (maxDist > eps) {
                keep[maxIndex] = true;
                if (maxIndex - start > 1) {
                    douglasPeucker(points, start, maxIndex, eps, keep);
                }
                if (end - maxIndex > 1) {
                    douglasPeucker(points, maxIndex, end, eps, keep);
                }
            }
        };
        
        // 标记要保留的点
        std::vector<bool> keepPoint(contourPoints.size(), false);
        keepPoint[0] = true;  // 保留起点
        keepPoint[contourPoints.size() - 1] = true;  // 保留终点
        
        // 执行Douglas-Peucker简化
        douglasPeucker(contourPoints, 0, contourPoints.size() - 1, epsilon, keepPoint);
        
        // 收集保留的点
        for (size_t i = 0; i < contourPoints.size(); i++) {
            if (keepPoint[i]) {
                simplifiedContour.push_back(contourPoints[i]);
            }
        }
        
        contourPoints = simplifiedContour;
        
        Logger::debug(QString("rasterizeIntersection: 轮廓点数从 %1 简化到 %2")
                     .arg(contourPixels.size())
                     .arg(contourPoints.size()));
    }
    
    // 创建路径
    if (!contourPoints.empty()) {
        resultPath.moveTo(contourPoints[0]);
        for (size_t i = 1; i < contourPoints.size(); ++i) {
            resultPath.lineTo(contourPoints[i]);
        }
        resultPath.closeSubpath();
    } else {
        Logger::warning("rasterizeIntersection: 轮廓跟踪失败，回退到传统方法");
        
        // 创建路径
        QBitmap bitmap = QBitmap::fromImage(resultImage.createAlphaMask());
        QRegion region(bitmap);
        resultPath.addRegion(region);
        
        // 应用反向变换
        QTransform inverseTransform = transform.inverted();
        resultPath = inverseTransform.map(resultPath);
    }
    
    Logger::debug(QString("rasterizeIntersection: 栅格化方法创建的路径包含 %1 个元素")
                 .arg(resultPath.elementCount()));
    
    return resultPath;
}

// 修改clipPath函数，增加栅格化备选方案
QPainterPath clipPath(const QPainterPath& subject, const QPainterPath& clip) {
    Logger::debug("clipPath: 开始裁剪路径");
    
    // 快速检查：如果路径为空，返回空路径
    if (subject.isEmpty() || clip.isEmpty()) {
        Logger::debug("clipPath: 主体或裁剪路径为空，返回空路径");
        return QPainterPath();
    }
    
    // 使用优化后的自定义路径交集算法
    Logger::debug("clipPath: 使用自定义交集算法计算路径裁剪");
    QPainterPath result = customIntersected(subject, clip);
    
    // 如果自定义算法失败，尝试使用栅格化方法
    if (result.isEmpty()) {
        Logger::debug("clipPath: 自定义交集算法失败，尝试使用栅格化方法");
        result = rasterizeIntersection(subject, clip);
    }
    
    return result;
}

} // namespace ClipAlgorithms 