#include "graphics_item_factory.h"
#include "circle_graphic_item.h"
#include "rectangle_graphic_item.h"
#include "line_graphic_item.h"
#include "ellipse_graphic_item.h"
#include "bezier_graphic_item.h"
// 后续可导入其他图形项类

QGraphicsItem* DefaultGraphicsItemFactory::createItem(GraphicItem::GraphicType type, const QPointF& position)
{
    switch (type) {
        case GraphicItem::CIRCLE:
            return new CircleGraphicItem(position, 50.0);
        
        case GraphicItem::RECTANGLE:
            return new RectangleGraphicItem(position, QSizeF(100, 60));
            
        case GraphicItem::LINE:
            return new LineGraphicItem(position, position + QPointF(100, 0));
        
        case GraphicItem::ELLIPSE:
            return new EllipseGraphicItem(position, 100.0, 60.0);
            
        case GraphicItem::BEZIER:
            // 创建默认的Bezier曲线，具有两个控制点
            return new BezierGraphicItem({position, position + QPointF(100, 0)});
            
        // 其他类型图形的创建，后续实现
        /*
        case GraphicItem::TRIANGLE:
            return new TriangleGraphicItem(position);
        */
            
        default:
            // 默认创建圆形
            return new CircleGraphicItem(position, 50.0);
    }
}

QGraphicsItem* DefaultGraphicsItemFactory::createCustomItem(GraphicItem::GraphicType type, const std::vector<QPointF>& points)
{
    if (points.empty()) {
        qDebug() << "创建图形失败: 点集为空";
        return nullptr;
    }
    
    switch (type) {
        case GraphicItem::CIRCLE:
            if (points.size() >= 2) {
                // 计算半径
                QPointF center = points[0];
                double radius = QLineF(center, points[1]).length();
                return new CircleGraphicItem(center, radius);
            }
            break;
            
        case GraphicItem::LINE:
            if (points.size() >= 2) {
                // 直接使用起点和终点创建线段
                return new LineGraphicItem(points[0], points[1]);
            }
            break;
            
        case GraphicItem::RECTANGLE:
            if (points.size() >= 2) {
                // 使用两点确定矩形，确保是标准化的矩形
                QRectF rect = QRectF(points[0], points[1]).normalized();
                return new RectangleGraphicItem(rect.topLeft(), rect.size());
            }
            break;
            
        case GraphicItem::ELLIPSE:
            if (points.size() >= 2) {
                // 创建标准化矩形，匹配预览椭圆的方式
                QRectF rect = QRectF(points[0], points[1]).normalized();
                
                // 计算中心点和尺寸
                QPointF center = rect.center();
                double width = rect.width();
                double height = rect.height();
                
                // 确保最小尺寸
                width = width < 1.0 ? 1.0 : width;
                height = height < 1.0 ? 1.0 : height;
                
                return new EllipseGraphicItem(center, width, height);
            }
            break;
            
        case GraphicItem::BEZIER:
            // 直接使用提供的控制点创建Bezier曲线
            if (points.size() >= 2) {
                return new BezierGraphicItem(points);
            }
            break;
            
        // 其他类型图形的自定义创建，后续实现
        /*
        case GraphicItem::TRIANGLE:
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