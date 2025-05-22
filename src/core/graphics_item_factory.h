#ifndef GRAPHICS_ITEM_FACTORY_H
#define GRAPHICS_ITEM_FACTORY_H

#include <QGraphicsItem>
#include <memory>
#include "graphic_item.h"
#include "line_graphic_item.h"
#include "rectangle_graphic_item.h"
#include "ellipse_graphic_item.h"
#include "circle_graphic_item.h"
#include "bezier_graphic_item.h"
#include "flowchart_process_item.h"
#include "flowchart_decision_item.h"
#include "flowchart_start_end_item.h"
#include "flowchart_io_item.h"
#include "flowchart_connector_item.h"
#include <vector>

// 流程图图元前向声明
class FlowchartProcessItem;
class FlowchartDecisionItem;
class FlowchartStartEndItem;
class FlowchartIOItem;
class FlowchartConnectorItem;

// 创建QGraphicsItem对象的工厂
class GraphicsItemFactory {
public:
    virtual ~GraphicsItemFactory() = default;
    
    // 创建基本图形项
    virtual QGraphicsItem* createItem(GraphicItem::GraphicType type, const QPointF& position) = 0;
    
    // 创建自定义图形项
    virtual QGraphicsItem* createCustomItem(GraphicItem::GraphicType type, const std::vector<QPointF>& points) = 0;
};

// 默认图形项工厂实现
class DefaultGraphicsItemFactory : public GraphicsItemFactory {
public:
    DefaultGraphicsItemFactory() = default;
    ~DefaultGraphicsItemFactory() override = default;
    
    QGraphicsItem* createItem(GraphicItem::GraphicType type, const QPointF& position) override;
    QGraphicsItem* createCustomItem(GraphicItem::GraphicType type, const std::vector<QPointF>& points) override;
    
    // 设置连接器类型和箭头类型
    void setConnectorType(FlowchartConnectorItem::ConnectorType type) { m_connectorType = type; }
    FlowchartConnectorItem::ConnectorType getConnectorType() const { return m_connectorType; }
    
    void setArrowType(FlowchartConnectorItem::ArrowType type) { m_arrowType = type; }
    FlowchartConnectorItem::ArrowType getArrowType() const { return m_arrowType; }
    
private:
    // 当前连接器类型和箭头类型设置
    FlowchartConnectorItem::ConnectorType m_connectorType = FlowchartConnectorItem::StraightLine;
    FlowchartConnectorItem::ArrowType m_arrowType = FlowchartConnectorItem::SingleArrow;
};

#endif // GRAPHICS_ITEM_FACTORY_H 