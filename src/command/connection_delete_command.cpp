#include "connection_delete_command.h"
#include "../core/connection_manager.h"
#include "../utils/logger.h"
#include <QApplication>

ConnectionDeleteCommand::ConnectionDeleteCommand(ConnectionManager* connectionManager,
                                               FlowchartConnectorItem* connector)
    : m_connectionManager(connectionManager)
    , m_connector(connector)
    , m_executed(false)
    , m_fromItem(nullptr)
    , m_fromPointIndex(-1)
    , m_toItem(nullptr)
    , m_toPointIndex(-1)
    , m_connectorType(FlowchartConnectorItem::StraightLine)
    , m_arrowType(FlowchartConnectorItem::SingleArrow)
{
    if (connector) {
        // 保存连接器的视觉属性
        m_startPoint = connector->getStartPoint();
        m_endPoint = connector->getEndPoint();
        m_connectorType = connector->getConnectorType();
        m_arrowType = connector->getArrowType();
        m_pen = connector->getPen();
        m_brush = connector->getBrush();
        
        // 保存连接信息
        saveConnectionInfo();
        
        Logger::debug(QString("ConnectionDeleteCommand: 创建连接删除命令 - 从 %1 到 %2")
            .arg(m_fromItem ? m_fromItem->getText() : "未知")
            .arg(m_toItem ? m_toItem->getText() : "未知"));
    } else {
        Logger::warning("ConnectionDeleteCommand: 传入空连接器指针");
    }
}

ConnectionDeleteCommand::~ConnectionDeleteCommand()
{
    // 如果命令已执行但连接器还在（撤销状态），需要清理
    if (m_executed && m_connector) {
        delete m_connector;
        m_connector = nullptr;
    }
    Logger::debug("ConnectionDeleteCommand: 销毁连接删除命令");
}

void ConnectionDeleteCommand::execute()
{
    if (m_executed || !m_connectionManager || !m_connector) {
        Logger::warning("ConnectionDeleteCommand::execute: 命令已执行或参数无效");
        return;
    }
    
    Logger::debug("ConnectionDeleteCommand::execute: 开始执行连接删除命令");
    
    try {
        // 移除连接
        m_connectionManager->removeConnection(m_connector);
        
        // 注意：removeConnection会删除连接器对象，所以我们标记为已删除
        m_connector = nullptr;
        
        m_executed = true;
        
        Logger::info(QString("ConnectionDeleteCommand::execute: 连接删除成功 - 从 %1 到 %2")
            .arg(m_fromItem ? m_fromItem->getText() : "已删除")
            .arg(m_toItem ? m_toItem->getText() : "已删除"));
            
    } catch (const std::exception& e) {
        Logger::error(QString("ConnectionDeleteCommand::execute: 异常 - %1").arg(e.what()));
    } catch (...) {
        Logger::error("ConnectionDeleteCommand::execute: 未知异常");
    }
}

void ConnectionDeleteCommand::undo()
{
    if (!m_executed || !m_connectionManager || !m_fromItem || !m_toItem) {
        Logger::debug("ConnectionDeleteCommand::undo: 命令未执行或参数无效");
        return;
    }
    
    Logger::debug("ConnectionDeleteCommand::undo: 开始撤销连接删除命令");
    
    try {
        // 重新创建连接
        bool success = m_connectionManager->createConnection(
            m_fromItem, m_fromPointIndex,
            m_toItem, m_toPointIndex,
            m_connectorType, m_arrowType
        );
        
        if (!success) {
            Logger::error("ConnectionDeleteCommand::undo: 重新创建连接失败");
            return;
        }
        
        // 查找重新创建的连接器
        QList<ConnectionManager::Connection> connections = m_connectionManager->getAllConnections();
        for (const auto& conn : connections) {
            if (conn.fromItem == m_fromItem && conn.fromPointIndex == m_fromPointIndex &&
                conn.toItem == m_toItem && conn.toPointIndex == m_toPointIndex) {
                m_connector = conn.connector;
                
                // 恢复连接器的视觉属性
                if (m_connector) {
                    m_connector->setPen(m_pen);
                    m_connector->setBrush(m_brush);
                }
                break;
            }
        }
        
        if (!m_connector) {
            Logger::warning("ConnectionDeleteCommand::undo: 找不到重新创建的连接器");
        }
        
        m_executed = false;
        
        Logger::info(QString("ConnectionDeleteCommand::undo: 撤销连接删除成功 - 从 %1 到 %2")
            .arg(m_fromItem ? m_fromItem->getText() : "已删除")
            .arg(m_toItem ? m_toItem->getText() : "已删除"));
            
    } catch (const std::exception& e) {
        Logger::error(QString("ConnectionDeleteCommand::undo: 异常 - %1").arg(e.what()));
        m_executed = false;
    } catch (...) {
        Logger::error("ConnectionDeleteCommand::undo: 未知异常");
        m_executed = false;
    }
}

QString ConnectionDeleteCommand::getDescription() const
{
    QString fromText = m_fromItem ? m_fromItem->getText() : "未知元素";
    QString toText = m_toItem ? m_toItem->getText() : "未知元素";
    
    // 如果文本太长，截断显示
    if (fromText.length() > 20) {
        fromText = fromText.left(17) + "...";
    }
    if (toText.length() > 20) {
        toText = toText.left(17) + "...";
    }
    
    return QString("删除连接: %1 → %2").arg(fromText).arg(toText);
}

QString ConnectionDeleteCommand::getType() const
{
    return "connection_delete";
}

void ConnectionDeleteCommand::saveConnectionInfo()
{
    if (!m_connectionManager || !m_connector) return;
    
    // 从连接管理器中查找对应的连接信息
    QList<ConnectionManager::Connection> connections = m_connectionManager->getAllConnections();
    for (const auto& conn : connections) {
        if (conn.connector == m_connector) {
            m_fromItem = conn.fromItem;
            m_fromPointIndex = conn.fromPointIndex;
            m_toItem = conn.toItem;
            m_toPointIndex = conn.toPointIndex;
            
            Logger::debug(QString("ConnectionDeleteCommand::saveConnectionInfo: 保存连接信息 - 从点%1到点%2")
                .arg(m_fromPointIndex).arg(m_toPointIndex));
            return;
        }
    }
    
    Logger::warning("ConnectionDeleteCommand::saveConnectionInfo: 在连接管理器中找不到对应的连接信息");
} 