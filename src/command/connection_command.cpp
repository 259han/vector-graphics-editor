#include "connection_command.h"
#include "../core/connection_manager.h"
#include "../utils/logger.h"
#include <QApplication>
#include <QDataStream>

ConnectionCommand::ConnectionCommand(ConnectionManager* connectionManager,
                                   FlowchartBaseItem* fromItem, int fromPointIndex,
                                   FlowchartBaseItem* toItem, int toPointIndex,
                                   FlowchartConnectorItem::ConnectorType connectorType,
                                   FlowchartConnectorItem::ArrowType arrowType)
    : m_connectionManager(connectionManager)
    , m_fromItem(fromItem)
    , m_fromPointIndex(fromPointIndex)
    , m_toItem(toItem)
    , m_toPointIndex(toPointIndex)
    , m_connectorType(connectorType)
    , m_arrowType(arrowType)
    , m_connector(nullptr)
    , m_executed(false)
    , m_fromPointWasOccupied(false)
    , m_toPointWasOccupied(false)
{
    Logger::debug(QString("ConnectionCommand: 创建连接命令 - 从 %1 到 %2")
        .arg(fromItem ? fromItem->getText() : "未知")
        .arg(toItem ? toItem->getText() : "未知"));
}

ConnectionCommand::~ConnectionCommand()
{
    Logger::debug("ConnectionCommand: 销毁连接命令");
}

void ConnectionCommand::execute()
{
    if (m_executed || !m_connectionManager || !m_fromItem || !m_toItem) {
        Logger::warning("ConnectionCommand::execute: 命令已执行或参数无效");
        return;
    }
    
    Logger::debug("ConnectionCommand::execute: 开始执行连接创建命令");
    
    const auto& fromPoints = m_connectionManager->getConnectionPointsData().value(m_fromItem);
    const auto& toPoints = m_connectionManager->getConnectionPointsData().value(m_toItem);
    
    if (m_fromPointIndex >= 0 && m_fromPointIndex < fromPoints.size()) {
        m_fromPointWasOccupied = fromPoints[m_fromPointIndex].isOccupied;
    }
    
    if (m_toPointIndex >= 0 && m_toPointIndex < toPoints.size()) {
        m_toPointWasOccupied = toPoints[m_toPointIndex].isOccupied;
    }
    
    bool success = m_connectionManager->createConnection(
        m_fromItem, m_fromPointIndex,
        m_toItem, m_toPointIndex,
        m_connectorType, m_arrowType
    );
    
    if (!success) {
        Logger::error("ConnectionCommand::execute: 连接创建失败");
        return;
    }
    
    QList<ConnectionManager::Connection> connections = m_connectionManager->getAllConnections();
    for (const auto& conn : connections) {
        if (conn.fromItem == m_fromItem && conn.fromPointIndex == m_fromPointIndex &&
            conn.toItem == m_toItem && conn.toPointIndex == m_toPointIndex) {
            m_connector = conn.connector;
            break;
        }
    }
    
    if (!m_connector) {
        Logger::error("ConnectionCommand::execute: 找不到创建的连接器");
        return;
    }
    
    m_executed = true;
    
    Logger::info(QString("ConnectionCommand::execute: 连接创建成功 - 从 %1 到 %2")
        .arg(m_fromItem->getText())
        .arg(m_toItem->getText()));
}

void ConnectionCommand::undo()
{
    if (!m_executed || !m_connectionManager || !m_connector) {
        Logger::debug("ConnectionCommand::undo: 命令未执行或连接器无效");
        return;
    }
    
    if (!m_fromItem || !m_toItem) {
        Logger::warning("ConnectionCommand::undo: 连接的流程图元素无效");
        m_executed = false;
        return;
    }
    
    Logger::debug("ConnectionCommand::undo: 开始撤销连接创建命令");
    
    try {
        m_connectionManager->removeConnection(m_connector);
        m_connector = nullptr;
        m_executed = false;
        Logger::info(QString("ConnectionCommand::undo: 撤销连接创建成功 - 从 %1 到 %2")
            .arg(m_fromItem ? m_fromItem->getText() : "已删除")
            .arg(m_toItem ? m_toItem->getText() : "已删除"));
    } catch (const std::exception& e) {
        Logger::error(QString("ConnectionCommand::undo: 异常 - %1").arg(e.what()));
        m_executed = false;
    } catch (...) {
        Logger::error("ConnectionCommand::undo: 未知异常");
        m_executed = false;
    }
}

QString ConnectionCommand::getDescription() const
{
    QString fromText = m_fromItem ? m_fromItem->getText() : "未知元素";
    QString toText = m_toItem ? m_toItem->getText() : "未知元素";
    if (fromText.length() > 20) {
        fromText = fromText.left(17) + "...";
    }
    if (toText.length() > 20) {
        toText = toText.left(17) + "...";
    }
    return QString("创建连接: %1 → %2").arg(fromText).arg(toText);
}

QString ConnectionCommand::getType() const
{
    return "connection";
}

void ConnectionCommand::serialize(QDataStream& out) const
{
    out << m_connector->uuid();
    out << (m_fromItem ? m_fromItem->uuid() : QUuid());
    out << m_fromPointIndex;
    out << (m_toItem ? m_toItem->uuid() : QUuid());
    out << m_toPointIndex;
    out << static_cast<int>(m_connectorType);
    out << static_cast<int>(m_arrowType);
}

void ConnectionCommand::deserialize(QDataStream& in)
{
    QUuid connectorUuid, fromUuid, toUuid;
    int connectorType, arrowType;
    
    in >> connectorUuid >> fromUuid >> m_fromPointIndex 
       >> toUuid >> m_toPointIndex >> connectorType >> arrowType;
    
    m_connectorType = static_cast<FlowchartConnectorItem::ConnectorType>(connectorType);
    m_arrowType = static_cast<FlowchartConnectorItem::ArrowType>(arrowType);
    
    m_pendingConnectorUuid = connectorUuid;
    m_pendingFromUuid = fromUuid;
    m_pendingToUuid = toUuid;
}
