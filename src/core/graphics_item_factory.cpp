#include "graphics_item_factory.h"
#include "circle_graphic_item.h"
#include "rectangle_graphic_item.h"
#include "line_graphic_item.h"
#include "ellipse_graphic_item.h"
#include "bezier_graphic_item.h"
#include "flowchart_process_item.h"
#include "flowchart_decision_item.h"
#include "flowchart_start_end_item.h"
#include "flowchart_io_item.h"
#include "flowchart_connector_item.h"
#include "concrete_flowchart_connector_item.h"
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
            
        // 流程图图元
        case GraphicItem::FLOWCHART_PROCESS:
            return new FlowchartProcessItem(position, QSizeF(120, 60));
            
        case GraphicItem::FLOWCHART_DECISION:
            return new FlowchartDecisionItem(position, QSizeF(120, 80));
            
        case GraphicItem::FLOWCHART_START_END:
            return new FlowchartStartEndItem(position, QSizeF(120, 60), true); // 默认为开始节点
            
        case GraphicItem::FLOWCHART_IO:
            return new FlowchartIOItem(position, QSizeF(120, 60), true); // 默认为输入节点
            
        case GraphicItem::FLOWCHART_CONNECTOR:
            // 创建默认的连接器，从当前位置到偏移位置
            return new ConcreteFlowchartConnectorItem(position, position + QPointF(100, 0), 
                                            FlowchartConnectorItem::StraightLine, 
                                            FlowchartConnectorItem::SingleArrow);
            
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

// 添加尺寸解析函数
static void parseSizeFromPoints(const std::vector<QPointF>& points, QPointF& center, QSizeF& size) {
    center = points[0];
    if (points[1].x() >= center.x() && points[1].y() >= center.y()) {
        size = QSizeF(
            (points[1].x() - center.x()) * 2,
            (points[1].y() - center.y()) * 2
        );
    } else {
        QRectF rect = QRectF(points[0], points[1]).normalized();
        center = rect.center();
        size = rect.size();
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
                QPointF center;
                QSizeF size;
                parseSizeFromPoints(points, center, size);
                return new RectangleGraphicItem(center - QPointF(size.width()/2, size.height()/2), size);
            }
            break;
            
        case GraphicItem::ELLIPSE:
            if (points.size() >= 2) {
                QPointF center;
                QSizeF size;
                parseSizeFromPoints(points, center, size);
                return new EllipseGraphicItem(center, size.width(), size.height());
            }
            break;
            
        case GraphicItem::BEZIER:
            // 直接使用提供的控制点创建Bezier曲线
            if (points.size() >= 2) {
                return new BezierGraphicItem(points);
            }
            break;
            
        // 流程图图元
        case GraphicItem::FLOWCHART_PROCESS:
            if (points.size() >= 2) {
                QPointF center;
                QSizeF size;
                parseSizeFromPoints(points, center, size);
                return new FlowchartProcessItem(center, size);
            }
            break;
            
        case GraphicItem::FLOWCHART_DECISION:
            if (points.size() >= 2) {
                QPointF center;
                QSizeF size;
                parseSizeFromPoints(points, center, size);
                return new FlowchartDecisionItem(center, size);
            }
            break;
            
        case GraphicItem::FLOWCHART_START_END:
            if (points.size() >= 2) {
                QPointF center;
                QSizeF size;
                parseSizeFromPoints(points, center, size);
                return new FlowchartStartEndItem(center, size, true); // 默认设置为"开始"
            }
            break;
            
        case GraphicItem::FLOWCHART_IO:
            if (points.size() >= 2) {
                QPointF center;
                QSizeF size;
                parseSizeFromPoints(points, center, size);
                return new FlowchartIOItem(center, size, true); // 默认设置为"输入"
            }
            break;
            
        case GraphicItem::FLOWCHART_CONNECTOR:
            if (points.size() >= 2) {
                // 创建连接器 - 使用当前设置的连接器类型和箭头类型
                ConcreteFlowchartConnectorItem* connector = new ConcreteFlowchartConnectorItem(
                    points[0], points[1], 
                    m_connectorType, 
                    m_arrowType);
                
                // 如果有更多点，作为控制点
                if (points.size() > 2) {
                    QList<QPointF> controlPoints;
                    for (size_t i = 2; i < points.size(); ++i) {
                        controlPoints.append(points[i]);
                    }
                    connector->setControlPoints(controlPoints);
                }
                
                return connector;
            }
            break;
            
        default:
            qDebug() << "创建图形失败: 不支持的图形类型" << type;
            return nullptr;
    }
    
    qDebug() << "创建图形失败: 点集数量不足";
    return nullptr;
} 