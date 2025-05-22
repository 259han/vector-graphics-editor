#ifndef CLIP_ALGORITHMS_H
#define CLIP_ALGORITHMS_H

#include <vector>
#include <QPointF>
#include <QRectF>
#include <QPainterPath>
#include <QPolygonF>

namespace ClipAlgorithms {

/**
 * @brief 使用Sutherland-Hodgman算法裁剪多边形
 * 
 * @param subjectPolygon 被裁剪的多边形顶点集合
 * @param clipRect 裁剪矩形
 * @return 裁剪后的多边形顶点集合
 */
std::vector<QPointF> sutherlandHodgmanClip(const std::vector<QPointF>& subjectPolygon, const QRectF& clipRect);

/**
 * @brief 使用增强版Weiler-Atherton算法裁剪任意多边形
 * 
 * @param subjectPolygon 被裁剪的多边形顶点集合
 * @param clipPolygon 裁剪多边形顶点集合
 * @return 裁剪后的多边形顶点集合
 */
std::vector<QPointF> weilerAthertonClip(const std::vector<QPointF>& subjectPolygon, 
                                        const std::vector<QPointF>& clipPolygon);

/**
 * @brief 自定义实现的路径交集算法，模仿Qt的intersected方法
 * 
 * @param subject 主体路径
 * @param clip 裁剪路径
 * @return 两者的交集路径
 */
QPainterPath customIntersected(const QPainterPath& subject, const QPainterPath& clip);

/**
 * @brief 使用栅格化方法计算两个路径的交集
 * 
 * @param subject 主体路径
 * @param clip 裁剪路径
 * @return 两者的交集路径
 */
QPainterPath rasterizeIntersection(const QPainterPath& subject, const QPainterPath& clip);

/**
 * @brief 裁剪一个路径，使用自定义方法代替Qt的intersected
 * 
 * @param subject 主体路径
 * @param clip 裁剪路径
 * @return 裁剪后的路径
 */
QPainterPath clipPath(const QPainterPath& subject, const QPainterPath& clip);

/**
 * @brief 将路径转换为点集合
 * 
 * @param path 要转换的路径
 * @param flatness 曲线的平滑度，值越小点越多
 * @return 路径上的点集合
 */
std::vector<QPointF> pathToPoints(const QPainterPath& path, qreal flatness = 0.5);

/**
 * @brief 将点集合转换为路径
 * 
 * @param points 点集合
 * @param closed 是否闭合路径
 * @return 由点集合形成的路径
 */
QPainterPath pointsToPath(const std::vector<QPointF>& points, bool closed = true);

/**
 * @brief 判断路径是否接近矩形
 * 
 * @param path 要检查的路径
 * @param tolerance 允许的误差
 * @return 如果路径接近矩形则返回true，否则返回false
 */
bool isPathRectangular(const QPainterPath& path, qreal tolerance = 0.05);

/**
 * @brief 裁剪线段
 * 
 * @param p1 线段起点，可能会被修改
 * @param p2 线段终点，可能会被修改
 * @param clipRect 裁剪矩形
 * @return 如果裁剪后线段仍然存在则返回true，否则返回false
 */
bool cohenSutherlandClip(QPointF& p1, QPointF& p2, const QRectF& clipRect);

// 裁剪边缘枚举
enum class ClipEdge {
    Left = 0,
    Right = 1,
    Bottom = 2,
    Top = 3
};

// 内部使用的辅助函数
namespace Internal {
    // Sutherland-Hodgman算法中处理单条边的裁剪
    std::vector<QPointF> clipPolygonToEdge(const std::vector<QPointF>& vertices, ClipEdge edge, const QRectF& clipRect);
    
    // 判断点是否在边的内部
    bool isInside(const QPointF& p, ClipEdge edge, const QRectF& clipRect);
    
    // 计算点与边的交点
    QPointF computeIntersection(const QPointF& p1, const QPointF& p2, ClipEdge edge, const QRectF& clipRect);
    
    // Cohen-Sutherland算法使用的区域编码
    int computeOutCode(const QPointF& p, const QRectF& clipRect);
    
    // 计算两线段的交点
    bool lineIntersection(const QPointF& p1, const QPointF& p2, 
                         const QPointF& p3, const QPointF& p4, 
                         QPointF& intersection);
    
    // 检查点是否在多边形内部
    bool pointInPolygon(const QPointF& point, const std::vector<QPointF>& polygon);
}

}

#endif // CLIP_ALGORITHMS_H 