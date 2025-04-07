#include "graphics_item_factory.h"
#include "circle_graphic_item.h"
#include "rectangle_graphic_item.h"
#include "line_graphic_item.h"
#include "ellipse_graphic_item.h"
#include "bezier_graphic_item.h"
// 后续可导入其他图形项类

QGraphicsItem* DefaultGraphicsItemFactory::createItem(Graphic::GraphicType type, const QPointF& position)
{
    switch (type) {
        case Graphic::CIRCLE:
            return new CircleGraphicItem(position, 50.0);
        
        case Graphic::RECTANGLE:
            return new RectangleGraphicItem(position, QSizeF(100, 60));
            
        case Graphic::LINE:
            return new LineGraphicItem(position, position + QPointF(100, 0));
        
        case Graphic::ELLIPSE:
            return new EllipseGraphicItem(position, 100.0, 60.0);
            
        case Graphic::BEZIER:
            // 创建默认的Bezier曲线，具有两个控制点
            return new BezierGraphicItem({position, position + QPointF(100, 0)});
            
        // 其他类型图形的创建，后续实现
        /*
        case Graphic::TRIANGLE:
            return new TriangleGraphicItem(position);
        */
            
        default:
            // 默认创建圆形
            return new CircleGraphicItem(position, 50.0);
    }
}

QGraphicsItem* DefaultGraphicsItemFactory::createCustomItem(Graphic::GraphicType type, const std::vector<QPointF>& points)
{
    if (points.empty()) {
        qDebug() << "创建图形失败: 点集为空";
        return nullptr;
    }
    
    switch (type) {
        case Graphic::CIRCLE:
            if (points.size() >= 2) {
                // 计算半径
                QPointF center = points[0];
                double radius = QLineF(center, points[1]).length();
                return new CircleGraphicItem(center, radius);
            }
            break;
            
        case Graphic::LINE:
            if (points.size() >= 2) {
                // 直接使用起点和终点创建线段
                return new LineGraphicItem(points[0], points[1]);
            }
            break;
            
        case Graphic::RECTANGLE:
            if (points.size() >= 2) {
                // 使用两点确定矩形，确保是标准化的矩形
                QRectF rect = QRectF(points[0], points[1]).normalized();
                return new RectangleGraphicItem(rect.topLeft(), rect.size());
            }
            break;
            
        case Graphic::ELLIPSE:
            if (points.size() >= 2) {
                QPointF center = points[0];
                QPointF radiusPoint = points[1];
                
                // 计算椭圆的半径 - 使用两点之间的差值作为半径
                double radiusX = std::abs(radiusPoint.x() - center.x());
                double radiusY = std::abs(radiusPoint.y() - center.y());
                
                // 确保半径不为零
                radiusX = radiusX < 1.0 ? 1.0 : radiusX;
                radiusY = radiusY < 1.0 ? 1.0 : radiusY;
                
                // 参数是宽度和高度，不是半径，所以需要乘以2
                return new EllipseGraphicItem(center, radiusX * 2, radiusY * 2);
            }
            break;
            
        case Graphic::BEZIER:
            // 直接使用提供的控制点创建Bezier曲线
            if (points.size() >= 2) {
                return new BezierGraphicItem(points);
            }
            break;
            
        // 其他类型图形的自定义创建，后续实现
        /*
        case Graphic::TRIANGLE:
            // 处理三角形
            break;
        */
            
        default:
            // 默认行为
            qDebug() << "未知的图形类型，无法创建";
            break;
    }
    
    return nullptr;
} 