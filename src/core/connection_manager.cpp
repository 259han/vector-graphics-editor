#include "connection_manager.h"
#include "flowchart_process_item.h"
#include "flowchart_decision_item.h"
#include "flowchart_start_end_item.h"
#include "flowchart_io_item.h"
#include "../utils/logger.h"
#include <QGraphicsScene>
#include <QtMath>
#include <QApplication>

ConnectionManager::ConnectionManager(QGraphicsScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_connectionPointsVisible(false)
    , m_currentVisibleItem(nullptr)
    , m_hasHighlight(false)
    , m_snapTolerance(20.0)
    , m_connectionPointSize(8.0)
{
    // 初始化更新定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(16); // 约60fps
    connect(m_updateTimer, &QTimer::timeout, this, &ConnectionManager::onUpdateTimer);
    
    // 初始化样式
    m_connectionPointColor = QColor(0, 120, 255, 180); // 半透明蓝色
    m_highlightColor = QColor(255, 120, 0, 200);        // 橙色高亮
    m_connectionPointPen = QPen(Qt::white, 2);
    m_connectionPointBrush = QBrush(m_connectionPointColor);
    
    Logger::info("ConnectionManager 初始化完成");
}

ConnectionManager::~ConnectionManager()
{
    clearAllConnectionPoints();
}

void ConnectionManager::registerFlowchartItem(FlowchartBaseItem* item)
{
    if (!item || m_connectionPoints.contains(item)) {
        return;
    }
    
    // 计算连接点，但使用延迟机制避免频繁重复计算
    if (!m_itemsToUpdate.contains(item)) {
        m_itemsToUpdate.append(item);
        if (!m_updateTimer->isActive()) {
            m_updateTimer->start();
        }
    }
    
    Logger::debug(QString("已注册流程图元素: %1").arg(item->getGraphicType()));
}

void ConnectionManager::unregisterFlowchartItem(FlowchartBaseItem* item)
{
    if (!item) return;
    
    // 移除所有相关连接
    removeAllConnectionsFor(item);
    
    // 移除连接点和缓存数据
    m_connectionPoints.remove(item);
    m_lastItemBounds.remove(item);
    m_lastConnectionCount.remove(item);
    
    // 如果当前正在显示这个项目的连接点，隐藏它们
    if (m_currentVisibleItem == item) {
        hideConnectionPoints();
    }
    
    Logger::debug(QString("已注销流程图元素: %1").arg(item->getGraphicType()));
}

void ConnectionManager::calculateConnectionPoints(FlowchartBaseItem* item)
{
    if (!item) return;
    
    // 快速检查：如果场景为空，直接返回
    if (!m_scene) {
        Logger::warning("ConnectionManager::calculateConnectionPoints: 场景为空，跳过计算");
        return;
    }
    
    // 优化：使用item->scene()而不是m_scene->items().contains(item)
    // 这样避免了遍历整个场景的开销
    if (!item->scene() || item->scene() != m_scene) {
        Logger::warning("ConnectionManager::calculateConnectionPoints: 元素不在场景中，跳过计算");
        return;
    }
    
    // 使用缓存检查是否需要重新计算
    QRectF currentBounds = item->sceneBoundingRect();
    QRectF lastBounds = m_lastItemBounds.value(item);
    
    // 如果边界矩形没有显著变化，跳过重新计算
    if (m_connectionPoints.contains(item) && !m_connectionPoints[item].isEmpty() && !lastBounds.isNull()) {
        const double TOLERANCE = 1.0; // 1像素容差
        if (qAbs(currentBounds.x() - lastBounds.x()) < TOLERANCE &&
            qAbs(currentBounds.y() - lastBounds.y()) < TOLERANCE &&
            qAbs(currentBounds.width() - lastBounds.width()) < TOLERANCE &&
            qAbs(currentBounds.height() - lastBounds.height()) < TOLERANCE) {
            // 位置和大小没有显著变化，跳过重新计算
            return;
        }
    }
    
    QList<ConnectionPoint> points;
    
    try {
        // 检查图形项是否已经完全初始化
        QRectF boundingRect = item->boundingRect();
        if (boundingRect.isEmpty()) {
            Logger::debug("ConnectionManager::calculateConnectionPoints: 图形项边界为空，延迟计算");
            // 重新安排计算
            if (!m_itemsToUpdate.contains(item)) {
                m_itemsToUpdate.append(item);
            }
            if (!m_updateTimer->isActive()) {
                m_updateTimer->setInterval(100); // 再次延迟
                m_updateTimer->start();
            }
            return;
        }
        
        // 获取项目的连接点（场景坐标）
        std::vector<QPointF> scenePoints = item->getConnectionPoints();
        
        // 预分配空间以提高性能
        points.reserve(scenePoints.size());
        
        for (int i = 0; i < scenePoints.size(); ++i) {
            QPointF scenePos = scenePoints[i];
            QPointF localPos = item->mapFromScene(scenePos);
            
            ConnectionPoint point(item, scenePos, localPos, i);
            points.append(point);
        }
        
        m_connectionPoints[item] = points;
        
        // 更新缓存
        m_lastItemBounds[item] = currentBounds;
        m_lastConnectionCount[item] = points.size();
        
        // 只在调试模式且连接点数量改变时输出日志
        static QMap<FlowchartBaseItem*, int> debugLogCounts;
        int lastDebugCount = debugLogCounts.value(item, -1);
        if (lastDebugCount != points.size()) {
            Logger::debug(QString("ConnectionManager::calculateConnectionPoints: 计算完成，%1个连接点")
                         .arg(points.size()));
            debugLogCounts[item] = points.size();
        }
    }
    catch (const std::exception& e) {
        Logger::error(QString("ConnectionManager::calculateConnectionPoints: 异常 - %1").arg(e.what()));
        // 如果计算失败，从管理器中移除该项目
        m_connectionPoints.remove(item);
        m_lastItemBounds.remove(item);
        m_lastConnectionCount.remove(item);
    }
    catch (...) {
        Logger::error("ConnectionManager::calculateConnectionPoints: 未知异常");
        // 如果计算失败，从管理器中移除该项目
        m_connectionPoints.remove(item);
        m_lastItemBounds.remove(item);
        m_lastConnectionCount.remove(item);
    }
}

void ConnectionManager::updateConnectionPoints(FlowchartBaseItem* item)
{
    if (!item) return;
    
    // 优化：使用缓存进行变化检测
    QRectF currentBounds = item->sceneBoundingRect();
    QRectF lastBounds = m_lastItemBounds.value(item);
    
    // 如果边界没有变化，跳过更新
    if (!lastBounds.isNull()) {
        const double TOLERANCE = 2.0; // 2像素容差
        if (qAbs(currentBounds.x() - lastBounds.x()) < TOLERANCE &&
            qAbs(currentBounds.y() - lastBounds.y()) < TOLERANCE &&
            qAbs(currentBounds.width() - lastBounds.width()) < TOLERANCE &&
            qAbs(currentBounds.height() - lastBounds.height()) < TOLERANCE) {
            // 变化很小，跳过更新
            return;
        }
    }
    
    // 重新计算连接点
    calculateConnectionPoints(item);
    
    // 安排更新（使用防重复机制）
    if (!m_itemsToUpdate.contains(item)) {
        m_itemsToUpdate.append(item);
    }
    
    if (!m_updateTimer->isActive()) {
        m_updateTimer->start();
    }
}

void ConnectionManager::showConnectionPoints(FlowchartBaseItem* item)
{
    if (!item) return;
    
    // 如果已经显示了同一个元素的连接点，无需重复操作
    if (m_currentVisibleItem == item && m_connectionPointsVisible) {
        return;
    }
    
    m_currentVisibleItem = item;
    m_connectionPointsVisible = true;
    
    // 确保连接点是最新的
    updateConnectionPoints(item);
    
    // 触发场景更新
    if (m_scene) {
        m_scene->update();
    }
    
    // 获取连接点数量用于调试
    int pointCount = 0;
    if (m_connectionPoints.contains(item)) {
        pointCount = m_connectionPoints[item].size();
    }
    
    Logger::debug(QString("显示 %1 的连接点，共 %2 个连接点").arg(item->getGraphicType()).arg(pointCount));
}

void ConnectionManager::hideConnectionPoints()
{
    if (!m_connectionPointsVisible) return;
    
    m_connectionPointsVisible = false;
    m_currentVisibleItem = nullptr;
    clearHighlight();
    
    // 触发场景更新
    if (m_scene) {
        m_scene->update();
    }
    
    Logger::debug("隐藏连接点");
}

void ConnectionManager::setConnectionPointsVisible(bool visible)
{
    if (visible && !m_currentVisibleItem) {
        // 如果要显示但没有指定项目，不做任何操作
        return;
    }
    
    if (visible) {
        showConnectionPoints(m_currentVisibleItem);
    } else {
        hideConnectionPoints();
    }
}

QPointF ConnectionManager::findNearestConnectionPoint(const QPointF& scenePos, FlowchartBaseItem* excludeItem)
{
    double minDistance = std::numeric_limits<double>::max();
    QPointF nearestPoint = scenePos;
    
    for (auto it = m_connectionPoints.begin(); it != m_connectionPoints.end(); ++it) {
        FlowchartBaseItem* item = it.key();
        if (item == excludeItem) continue;
        
        const QList<ConnectionPoint>& points = it.value();
        for (const ConnectionPoint& point : points) {
            double distance = QLineF(scenePos, point.scenePos).length();
            if (distance < minDistance && distance <= m_snapTolerance) {
                minDistance = distance;
                nearestPoint = point.scenePos;
            }
        }
    }
    
    return nearestPoint;
}

ConnectionManager::ConnectionPoint* ConnectionManager::findConnectionPointAt(const QPointF& scenePos, double tolerance)
{
    for (auto it = m_connectionPoints.begin(); it != m_connectionPoints.end(); ++it) {
        QList<ConnectionPoint>& points = it.value();
        for (ConnectionPoint& point : points) {
            double distance = QLineF(scenePos, point.scenePos).length();
            if (distance <= tolerance) {
                return &point;
            }
        }
    }
    return nullptr;
}

bool ConnectionManager::isNearConnectionPoint(const QPointF& scenePos, double tolerance)
{
    return findConnectionPointAt(scenePos, tolerance) != nullptr;
}

bool ConnectionManager::createConnection(FlowchartBaseItem* fromItem, int fromPointIndex, 
                                       FlowchartBaseItem* toItem, int toPointIndex,
                                       FlowchartConnectorItem::ConnectorType connectorType,
                                       FlowchartConnectorItem::ArrowType arrowType)
{
    if (!fromItem || !toItem || fromItem == toItem) {
        Logger::warning("无效的连接参数");
        return false;
    }
    
    // 验证元素是否在场景中
    if (!m_scene || !m_scene->items().contains(fromItem) || !m_scene->items().contains(toItem)) {
        Logger::warning("连接的元素不在场景中");
        return false;
    }
    
    if (!canConnect(fromItem, toItem)) {
        Logger::warning("无法连接这两个元素");
        return false;
    }
    
    // 获取连接点
    QList<ConnectionPoint> fromPoints = m_connectionPoints.value(fromItem);
    QList<ConnectionPoint> toPoints = m_connectionPoints.value(toItem);
    
    if (fromPointIndex < 0 || fromPointIndex >= fromPoints.size() ||
        toPointIndex < 0 || toPointIndex >= toPoints.size()) {
        Logger::warning("连接点索引无效");
        return false;
    }
    
    QPointF startPos = fromPoints[fromPointIndex].scenePos;
    QPointF endPos = toPoints[toPointIndex].scenePos;
    
    // 创建连接器
    FlowchartConnectorItem* connector = new FlowchartConnectorItem(startPos, endPos, connectorType, arrowType);
    
    // 添加到场景
    if (m_scene) {
        m_scene->addItem(connector);
    }
    
    // 记录连接关系
    Connection connection(fromItem, fromPointIndex, toItem, toPointIndex, connector);
    m_connections.append(connection);
    
    // 标记连接点为已占用
    m_connectionPoints[fromItem][fromPointIndex].isOccupied = true;
    m_connectionPoints[toItem][toPointIndex].isOccupied = true;
    
    // 发射信号
    emit connectionCreated(fromItem, toItem, connector);
    
    Logger::info(QString("成功创建连接: %1 -> %2").arg(fromItem->getGraphicType()).arg(toItem->getGraphicType()));
    return true;
}

void ConnectionManager::removeConnection(FlowchartConnectorItem* connector)
{
    if (!connector) return;
    
    // 查找并移除连接记录
    for (int i = 0; i < m_connections.size(); ++i) {
        if (m_connections[i].connector == connector) {
            Connection& conn = m_connections[i];
            
            // 释放连接点
            if (m_connectionPoints.contains(conn.fromItem) && 
                conn.fromPointIndex >= 0 && conn.fromPointIndex < m_connectionPoints[conn.fromItem].size()) {
                m_connectionPoints[conn.fromItem][conn.fromPointIndex].isOccupied = false;
            }
            
            if (m_connectionPoints.contains(conn.toItem) && 
                conn.toPointIndex >= 0 && conn.toPointIndex < m_connectionPoints[conn.toItem].size()) {
                m_connectionPoints[conn.toItem][conn.toPointIndex].isOccupied = false;
            }
            
            // 从场景移除连接器
            if (m_scene && m_scene->items().contains(connector)) {
                m_scene->removeItem(connector);
            }
            
            // 移除连接记录
            m_connections.removeAt(i);
            
            // 发射信号
            emit connectionRemoved(connector);
            
            Logger::info("移除了一个连接");
            break;
        }
    }
    
    // 删除连接器对象
    delete connector;
}

void ConnectionManager::removeAllConnectionsFor(FlowchartBaseItem* item)
{
    if (!item) return;
    
    QList<FlowchartConnectorItem*> connectorsToRemove;
    
    for (const Connection& conn : m_connections) {
        if (conn.fromItem == item || conn.toItem == item) {
            connectorsToRemove.append(conn.connector);
        }
    }
    
    for (FlowchartConnectorItem* connector : connectorsToRemove) {
        removeConnection(connector);
    }
}

void ConnectionManager::updateConnections(FlowchartBaseItem* item)
{
    if (!item || !m_scene) return;
    
    // 检查元素是否仍在场景中
    if (!m_scene->items().contains(item)) {
        Logger::warning("ConnectionManager::updateConnections: 元素不在场景中，跳过更新");
        return;
    }
    
    // 首先更新连接点位置（仅当连接点已存在时）
    if (m_connectionPoints.contains(item) && !m_connectionPoints[item].isEmpty()) {
        updateConnectionPoints(item);
    }
    
    // 批量收集需要更新的连接器，避免重复更新
    QSet<FlowchartConnectorItem*> connectorsToUpdate;
    
    // 使用迭代器安全地遍历和移除无效连接
    auto it = m_connections.begin();
    while (it != m_connections.end()) {
        Connection& conn = *it;
        
        if (conn.fromItem == item || conn.toItem == item) {
            // 简化的安全检查：先检查场景包含，再检查scene()
            bool fromValid = conn.fromItem && m_scene->items().contains(conn.fromItem) && 
                            conn.fromItem->scene() == m_scene;
            bool toValid = conn.toItem && m_scene->items().contains(conn.toItem) && 
                          conn.toItem->scene() == m_scene;
            
            if (!fromValid || !toValid) {
                // 移除无效连接
                if (conn.connector && m_scene->items().contains(conn.connector)) {
                    m_scene->removeItem(conn.connector);
                    delete conn.connector;
                }
                it = m_connections.erase(it);
                continue;
            }
            
            // 获取连接点位置并更新连接器
            if (conn.fromItem && m_connectionPoints.contains(conn.fromItem) && 
                conn.fromPointIndex >= 0 && conn.fromPointIndex < m_connectionPoints[conn.fromItem].size() &&
                conn.toItem && m_connectionPoints.contains(conn.toItem) && 
                conn.toPointIndex >= 0 && conn.toPointIndex < m_connectionPoints[conn.toItem].size() &&
                conn.connector) {
                connectorsToUpdate.insert(conn.connector);
            }
        }
        
        ++it;
    }
    
    // 批量更新连接器
    for (FlowchartConnectorItem* connector : connectorsToUpdate) {
        for (const Connection& conn : m_connections) {
            if (conn.connector == connector) {
                QPointF startPos = m_connectionPoints[conn.fromItem][conn.fromPointIndex].scenePos;
                QPointF endPos = m_connectionPoints[conn.toItem][conn.toPointIndex].scenePos;
                
                connector->setStartPoint(startPos);
                connector->setEndPoint(endPos);
                break;
            }
        }
    }
}

void ConnectionManager::updateAllConnections()
{
    for (auto it = m_connectionPoints.begin(); it != m_connectionPoints.end(); ++it) {
        updateConnections(it.key());
    }
}

bool ConnectionManager::canConnect(FlowchartBaseItem* fromItem, FlowchartBaseItem* toItem) const
{
    if (!fromItem || !toItem || fromItem == toItem) {
        return false;
    }
    
    // 检查是否已经连接
    if (isConnected(fromItem, toItem)) {
        return false;
    }
    
    // 可以添加更多连接规则，比如防止循环连接等
    return true;
}

bool ConnectionManager::isConnected(FlowchartBaseItem* fromItem, FlowchartBaseItem* toItem) const
{
    if (!fromItem || !toItem) {
        return false;
    }
    
    for (const Connection& conn : m_connections) {
        if ((conn.fromItem == fromItem && conn.toItem == toItem) ||
            (conn.fromItem == toItem && conn.toItem == fromItem)) {
            return true;
        }
    }
    return false;
}

QList<ConnectionManager::Connection> ConnectionManager::getConnectionsFor(FlowchartBaseItem* item) const
{
    QList<Connection> result;
    
    if (!item) {
        return result;
    }
    
    for (const Connection& conn : m_connections) {
        if (conn.fromItem == item || conn.toItem == item) {
            result.append(conn);
        }
    }
    return result;
}

void ConnectionManager::highlightConnectionPoint(const ConnectionPoint& point)
{
    // 检查是否是同一个连接点，避免重复高亮
    if (m_hasHighlight && 
        m_highlightedPoint.item == point.item && 
        m_highlightedPoint.index == point.index) {
        return; // 已经高亮了同一个点，无需更新
    }
    
    m_highlightedPoint = point;
    m_hasHighlight = true;
    
    // 发射悬停信号
    emit connectionPointHovered(point);
    
    // 使用延迟更新而不是立即全场景更新，提高流畅度
    // 只更新连接点周围的小区域
    if (m_scene) {
        const double updateRadius = m_connectionPointSize * 2; // 只更新连接点周围区域
        QRectF updateRect(point.scenePos.x() - updateRadius, 
                         point.scenePos.y() - updateRadius,
                         updateRadius * 2, updateRadius * 2);
        m_scene->update(updateRect);
    }
}

void ConnectionManager::clearHighlight()
{
    if (!m_hasHighlight) return;
    
    // 保存旧的高亮位置用于局部更新
    QPointF oldHighlightPos = m_highlightedPoint.scenePos;
    
    m_hasHighlight = false;
    
    // 只更新之前高亮点的区域，而不是全场景
    if (m_scene) {
        const double updateRadius = m_connectionPointSize * 2;
        QRectF updateRect(oldHighlightPos.x() - updateRadius, 
                         oldHighlightPos.y() - updateRadius,
                         updateRadius * 2, updateRadius * 2);
        m_scene->update(updateRect);
    }
    
    Logger::debug("ConnectionManager: 清除连接点高亮");
}

void ConnectionManager::clearAllConnectionPoints()
{
    // 移除所有连接
    QList<FlowchartConnectorItem*> connectorsToRemove;
    for (const Connection& conn : m_connections) {
        connectorsToRemove.append(conn.connector);
    }
    
    for (FlowchartConnectorItem* connector : connectorsToRemove) {
        removeConnection(connector);
    }
    
    // 清空数据和缓存
    m_connectionPoints.clear();
    m_connections.clear();
    m_lastItemBounds.clear();
    m_lastConnectionCount.clear();
    hideConnectionPoints();
}

void ConnectionManager::cleanupInvalidItems()
{
    if (!m_scene) return;
    
    // 收集无效的流程图元素
    QList<FlowchartBaseItem*> invalidItems;
    
    for (auto it = m_connectionPoints.begin(); it != m_connectionPoints.end(); ++it) {
        FlowchartBaseItem* item = it.key();
        // 简化检查：指针为空或不在场景中就认为无效
        if (!item || !m_scene->items().contains(item)) {
            invalidItems.append(item);
        }
    }
    
    // 移除无效项目和相关缓存
    for (FlowchartBaseItem* item : invalidItems) {
        m_connectionPoints.remove(item);
        m_lastItemBounds.remove(item);
        m_lastConnectionCount.remove(item);
        
        if (m_currentVisibleItem == item) {
            hideConnectionPoints();
        }
    }
    
    // 清理无效的连接
    QList<FlowchartConnectorItem*> invalidConnectors;
    
    for (const Connection& conn : m_connections) {
        bool fromInvalid = !conn.fromItem || !m_scene->items().contains(conn.fromItem);
        bool toInvalid = !conn.toItem || !m_scene->items().contains(conn.toItem);
        bool connectorInvalid = !conn.connector || !m_scene->items().contains(conn.connector);
        
        if (fromInvalid || toInvalid || connectorInvalid) {
            invalidConnectors.append(conn.connector);
        }
    }
    
    for (FlowchartConnectorItem* connector : invalidConnectors) {
        removeConnection(connector);
    }
    
    // 清理待更新列表
    m_itemsToUpdate.removeAll(nullptr);
    QList<FlowchartBaseItem*> validItemsToUpdate;
    for (FlowchartBaseItem* item : m_itemsToUpdate) {
        if (item && m_scene->items().contains(item)) {
            validItemsToUpdate.append(item);
        }
    }
    m_itemsToUpdate = validItemsToUpdate;
    
    // 只有在实际清理了内容时才输出日志
    if (invalidItems.size() > 0 || invalidConnectors.size() > 0) {
        Logger::info(QString("ConnectionManager清理完成: 移除了%1个无效项目，%2个无效连接")
                     .arg(invalidItems.size()).arg(invalidConnectors.size()));
    }
}

void ConnectionManager::onItemPositionChanged()
{
    // 这个方法现在不使用信号槽，而是通过定时器定期更新
    // 或者通过外部调用updateConnections来更新
}

void ConnectionManager::onUpdateTimer()
{
    // 如果待更新列表为空，直接返回
    if (m_itemsToUpdate.isEmpty()) {
        return;
    }
    
    // 处理待更新的项目，过滤掉无效和重复的项目
    QSet<FlowchartBaseItem*> uniqueItemsSet; // 使用Set去重
    QList<FlowchartBaseItem*> validItemsToUpdate;
    bool hasInvalidItems = false;
    
    for (FlowchartBaseItem* item : m_itemsToUpdate) {
        if (item && item->scene() == m_scene) {
            // 使用Set去重，避免重复处理同一个item
            if (!uniqueItemsSet.contains(item)) {
                uniqueItemsSet.insert(item);
                validItemsToUpdate.append(item);
            }
        } else {
            hasInvalidItems = true;
        }
    }
    
    // 只有在发现无效项目时才进行清理，减少不必要的清理操作
    if (hasInvalidItems) {
        cleanupInvalidItems();
    }
    
    // 批量更新有效的项目，限制每次更新的数量避免阻塞
    const int MAX_ITEMS_PER_BATCH = 5; // 每批最多处理5个项目
    int processedCount = 0;
    bool needsSceneUpdate = false;
    QList<FlowchartBaseItem*> remainingItems;
    
    for (FlowchartBaseItem* item : validItemsToUpdate) {
        if (processedCount >= MAX_ITEMS_PER_BATCH) {
            // 超过批处理限制，将剩余项目留到下次处理
            remainingItems.append(item);
            continue;
        }
        
        // 检查是否需要计算连接点
        if (!m_connectionPoints.contains(item) || m_connectionPoints[item].isEmpty()) {
            calculateConnectionPoints(item);
            needsSceneUpdate = true;
            processedCount++;
        } else {
            // 只有在连接真正存在时才更新连接
            QList<Connection> connections = getConnectionsFor(item);
            if (!connections.isEmpty()) {
                updateConnections(item);
                processedCount++;
            }
        }
    }
    
    // 清空当前待更新列表，设置剩余项目
    m_itemsToUpdate = remainingItems;
    
    // 如果还有剩余项目需要处理，重新启动定时器
    if (!m_itemsToUpdate.isEmpty()) {
        // 使用较短的间隔继续处理剩余项目
        m_updateTimer->setInterval(32); // 约30fps
        m_updateTimer->start();
    } else {
        // 重置定时器间隔为正常值
        m_updateTimer->setInterval(16);
    }
    
    // 只在必要时触发场景更新，避免频繁重绘
    if (needsSceneUpdate && m_scene) {
        // 使用QTimer::singleShot延迟场景更新，避免阻塞UI线程
        QTimer::singleShot(0, [this]() {
            if (m_scene) {
                m_scene->update();
            }
        });
    }
} 