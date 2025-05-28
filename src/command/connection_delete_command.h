#ifndef CONNECTION_DELETE_COMMAND_H
#define CONNECTION_DELETE_COMMAND_H

#include "command.h"
#include "../core/flowchart_base_item.h"
#include "../core/flowchart_connector_item.h"
#include <QGraphicsScene>

class ConnectionManager;

/**
 * @brief 连接删除命令类
 * 
 * 处理流程图元素间连接删除操作的撤销/重做功能
 */
class ConnectionDeleteCommand : public Command {
public:
    /**
     * @brief 构造函数
     * @param connectionManager 连接管理器
     * @param connector 要删除的连接器
     */
    ConnectionDeleteCommand(ConnectionManager* connectionManager,
                           FlowchartConnectorItem* connector);
    
    /**
     * @brief 析构函数
     */
    ~ConnectionDeleteCommand() override;
    
    /**
     * @brief 执行连接删除命令
     */
    void execute() override;
    
    /**
     * @brief 撤销连接删除命令（重新创建连接）
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

private:
    ConnectionManager* m_connectionManager;
    FlowchartConnectorItem* m_connector;
    bool m_executed;
    
    // 保存连接信息用于撤销
    FlowchartBaseItem* m_fromItem;
    int m_fromPointIndex;
    FlowchartBaseItem* m_toItem;
    int m_toPointIndex;
    FlowchartConnectorItem::ConnectorType m_connectorType;
    FlowchartConnectorItem::ArrowType m_arrowType;
    
    // 保存连接器的视觉属性
    QPointF m_startPoint;
    QPointF m_endPoint;
    QPen m_pen;
    QBrush m_brush;
    
    // 从连接管理器获取连接信息
    void saveConnectionInfo();
};

#endif // CONNECTION_DELETE_COMMAND_H 