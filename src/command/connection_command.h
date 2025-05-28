#ifndef CONNECTION_COMMAND_H
#define CONNECTION_COMMAND_H

#include "command.h"
#include "../core/flowchart_base_item.h"
#include "../core/flowchart_connector_item.h"
#include <QGraphicsScene>

class ConnectionManager;

/**
 * @brief 连接创建命令类
 * 
 * 处理流程图元素间连接创建操作的撤销/重做功能
 */
class ConnectionCommand : public Command {
public:
    /**
     * @brief 构造函数
     * @param connectionManager 连接管理器
     * @param fromItem 起始流程图元素
     * @param fromPointIndex 起始连接点索引
     * @param toItem 目标流程图元素
     * @param toPointIndex 目标连接点索引
     * @param connectorType 连接器类型
     * @param arrowType 箭头类型
     */
    ConnectionCommand(ConnectionManager* connectionManager,
                     FlowchartBaseItem* fromItem, int fromPointIndex,
                     FlowchartBaseItem* toItem, int toPointIndex,
                     FlowchartConnectorItem::ConnectorType connectorType = FlowchartConnectorItem::StraightLine,
                     FlowchartConnectorItem::ArrowType arrowType = FlowchartConnectorItem::SingleArrow);
    
    /**
     * @brief 析构函数
     */
    ~ConnectionCommand() override;
    
    /**
     * @brief 执行连接创建命令
     */
    void execute() override;
    
    /**
     * @brief 撤销连接创建命令
     */
    void undo() override;
    
    /**
     * @brief 获取命令描述
     * @return 命令描述字符串
     */
    QString getDescription() const override;
    
    /**
     * @brief 获取命令类型
     * @return 命令类型字符串
     */
    QString getType() const override;
    
    /**
     * @brief 获取创建的连接器
     * @return 连接器指针，如果未创建则返回nullptr
     */
    FlowchartConnectorItem* getConnector() const { return m_connector; }

private:
    ConnectionManager* m_connectionManager;
    FlowchartBaseItem* m_fromItem;
    int m_fromPointIndex;
    FlowchartBaseItem* m_toItem;
    int m_toPointIndex;
    FlowchartConnectorItem::ConnectorType m_connectorType;
    FlowchartConnectorItem::ArrowType m_arrowType;
    FlowchartConnectorItem* m_connector;
    bool m_executed;
    
    // 保存连接创建前的状态信息，用于撤销
    bool m_fromPointWasOccupied;
    bool m_toPointWasOccupied;
};

#endif // CONNECTION_COMMAND_H 