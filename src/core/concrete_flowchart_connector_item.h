#ifndef CONCRETE_FLOWCHART_CONNECTOR_ITEM_H
#define CONCRETE_FLOWCHART_CONNECTOR_ITEM_H

#include "flowchart_connector_item.h"

/**
 * @brief 具体的流程图连接线实现类
 */
class ConcreteFlowchartConnectorItem : public FlowchartConnectorItem {
public:
    ConcreteFlowchartConnectorItem(const QPointF& startPoint = QPointF(), 
                                 const QPointF& endPoint = QPointF(),
                                 ConnectorType type = StraightLine,
                                 ArrowType arrowType = SingleArrow)
        : FlowchartConnectorItem(startPoint, endPoint, type, arrowType) {}
    
    ~ConcreteFlowchartConnectorItem() override = default;
};

#endif // CONCRETE_FLOWCHART_CONNECTOR_ITEM_H 