#include "auto_connect_state.h"
#include "../ui/draw_area.h"
#include "../core/connection_point_overlay.h"
#include "../utils/logger.h"
#include "../command/command_manager.h"
#include "../command/connection_command.h"
#include <QMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QCursor>
#include <QtMath>
#include <QTimer>

AutoConnectState::AutoConnectState()
    : EditorState()
    , m_isConnecting(false)
    , m_sourceItem(nullptr)
    , m_sourcePointIndex(-1)
    , m_previewConnector(nullptr)
    , m_connectorType(FlowchartConnectorItem::StraightLine)
    , m_arrowType(FlowchartConnectorItem::SingleArrow)
    , m_lastHoveredItem(nullptr)
    , m_lastHoveredPoint(nullptr)
    , m_currentCursor(Qt::CrossCursor)
{
    // 初始化悬停更新定时器
    m_hoverUpdateTimer = new QTimer();
    m_hoverUpdateTimer->setSingleShot(true);
    m_hoverUpdateTimer->setInterval(16); // 约60fps，平滑更新
    
    Logger::debug("AutoConnectState: 自动连接状态创建");
}

AutoConnectState::~AutoConnectState()
{
    delete m_hoverUpdateTimer;
    Logger::debug("AutoConnectState: 自动连接状态销毁");
}

void AutoConnectState::onEnterState(DrawArea* drawArea)
{
    if (!drawArea) return;
    
    // 设置连接光标
    drawArea->setCursor(QCursor(Qt::CrossCursor));
    m_currentCursor = Qt::CrossCursor;
    
    // 重置状态和缓存
    m_isConnecting = false;
    m_sourceItem = nullptr;
    m_sourcePointIndex = -1;
    m_previewConnector = nullptr;
    m_lastHoveredItem = nullptr;
    m_lastHoveredPoint = nullptr;
    
    Logger::info("AutoConnectState: 进入自动连接模式");
}

void AutoConnectState::onExitState(DrawArea* drawArea)
{
    if (!drawArea) return;
    
    // 取消当前连接
    cancelConnection(drawArea);
    
    // 确保所有连接点和高亮都被清除
    ConnectionManager* connectionManager = drawArea->getConnectionManager();
    if (connectionManager) {
        connectionManager->hideConnectionPoints();
        connectionManager->clearHighlight();
    }
    
    // 确保连接点覆盖层也清除高亮
    ConnectionPointOverlay* overlay = drawArea->getConnectionOverlay();
    if (overlay) {
        overlay->clearHighlight();
        overlay->setConnectionPointsVisible(false);
    }
    
    // 重置缓存
    m_lastHoveredItem = nullptr;
    m_lastHoveredPoint = nullptr;
    
    // 恢复默认光标
    drawArea->setCursor(QCursor(Qt::ArrowCursor));
    m_currentCursor = Qt::ArrowCursor;
    
    Logger::info("AutoConnectState: 退出自动连接模式");
}

void AutoConnectState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event)
{
    if (!drawArea || !event) return;
    
    QPointF scenePos = drawArea->mapToScene(event->pos());
    
    if (event->button() == Qt::LeftButton) {
        handleLeftMousePress(drawArea, scenePos);
    } else if (event->button() == Qt::RightButton) {
        handleRightMousePress(drawArea, scenePos);
    }
    
    event->accept();
}

void AutoConnectState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event)
{
    if (!drawArea || !event) return;
    
    QPointF scenePos = drawArea->mapToScene(event->pos());
    m_currentPos = scenePos;
    
    if (m_isConnecting) {
        updateConnectionPreview(drawArea, scenePos);
    } else {
        // 使用去抖动机制，避免频繁更新
        // 首先进行快速的缓存检查
        FlowchartBaseItem* item = findFlowchartItemAt(drawArea, scenePos);
        ConnectionManager* connectionManager = drawArea->getConnectionManager();
        
        if (!connectionManager) {
            event->accept();
            return;
        }
        
        // 快速检查是否还在同一个元素上
        if (item == m_lastHoveredItem && item) {
            // 仍在同一个元素上，检查连接点
            ConnectionManager::ConnectionPoint* point = 
                connectionManager->findConnectionPointAt(scenePos, 15.0);
            
            if (point == m_lastHoveredPoint) {
                // 悬停状态没有变化，无需更新
                event->accept();
                return;
            }
            
            // 连接点变化了，更新高亮
            m_lastHoveredPoint = point;
            if (point && !point->isOccupied) {
                connectionManager->highlightConnectionPoint(*point);
                setCursorSmoothly(drawArea, Qt::PointingHandCursor);
            } else {
                connectionManager->clearHighlight();
                setCursorSmoothly(drawArea, Qt::CrossCursor);
            }
        } else {
            // 切换到了不同的元素，进行完整更新
            m_lastHoveredItem = item;
            m_lastHoveredPoint = nullptr;
            
            if (item) {
                connectionManager->showConnectionPoints(item);
                
                // 检查是否悬停在连接点上
                ConnectionManager::ConnectionPoint* point = 
                    connectionManager->findConnectionPointAt(scenePos, 15.0);
                m_lastHoveredPoint = point;
                
                if (point && !point->isOccupied) {
                    connectionManager->highlightConnectionPoint(*point);
                    setCursorSmoothly(drawArea, Qt::PointingHandCursor);
                } else {
                    connectionManager->clearHighlight();
                    setCursorSmoothly(drawArea, Qt::CrossCursor);
                }
            } else {
                connectionManager->hideConnectionPoints();
                connectionManager->clearHighlight();
                setCursorSmoothly(drawArea, Qt::CrossCursor);
            }
        }
    }
    
    event->accept();
}

void AutoConnectState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event)
{
    if (!drawArea || !event) return;
    
    if (event->button() == Qt::LeftButton && m_isConnecting) {
        QPointF scenePos = drawArea->mapToScene(event->pos());
        finishConnection(drawArea, scenePos);
    }
    
    event->accept();
}

void AutoConnectState::paintEvent(DrawArea* drawArea, QPainter* painter)
{
    Q_UNUSED(drawArea)
    Q_UNUSED(painter)
    // AutoConnect状态不需要特殊的绘制
}

void AutoConnectState::keyPressEvent(DrawArea* drawArea, QKeyEvent* event)
{
    if (!drawArea || !event) return;
    
    // 处理ESC键取消连接
    if (event->key() == Qt::Key_Escape) {
        if (m_isConnecting) {
            cancelConnection(drawArea);
            event->accept();
            return;
        }
    }
    
    // 其他键盘事件可以传递给基类处理
    EditorState::handleCommonKeyboardShortcuts(drawArea, event);
}

void AutoConnectState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    if (!drawArea) return;
    
    if (!m_isConnecting) {
        // 开始连接
        FlowchartBaseItem* item = findFlowchartItemAt(drawArea, scenePos);
        if (item) {
            ConnectionManager* connectionManager = drawArea->getConnectionManager();
            if (connectionManager) {
                ConnectionManager::ConnectionPoint* point = 
                    connectionManager->findConnectionPointAt(scenePos, 15.0);
                if (point && !point->isOccupied) {
                    startConnection(drawArea, item, point->index, point->scenePos);
                    Logger::info(QString("开始从 %1 创建连接").arg(item->getText()));
                }
            }
        }
    } else {
        // 完成连接（这种情况通常在mouseReleaseEvent中处理）
        finishConnection(drawArea, scenePos);
    }
}

void AutoConnectState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    Q_UNUSED(scenePos)
    
    if (m_isConnecting) {
        // 右键取消连接
        cancelConnection(drawArea);
        Logger::info("用户取消连接创建");
    }
}

void AutoConnectState::setConnectorType(FlowchartConnectorItem::ConnectorType type)
{
    m_connectorType = type;
}

void AutoConnectState::setArrowType(FlowchartConnectorItem::ArrowType type)
{
    m_arrowType = type;
}

void AutoConnectState::startConnection(DrawArea* drawArea, FlowchartBaseItem* sourceItem, int pointIndex, const QPointF& startPos)
{
    if (!drawArea || !sourceItem) return;
    
    m_isConnecting = true;
    m_sourceItem = sourceItem;
    m_sourcePointIndex = pointIndex;
    m_startPos = startPos;
    m_currentPos = startPos;
    
    // 创建预览连接器
    createPreviewConnector(drawArea);
    
    // 更新光标
    setCursorSmoothly(drawArea, Qt::CrossCursor);
}

void AutoConnectState::updateConnectionPreview(DrawArea* drawArea, const QPointF& currentPos)
{
    if (!drawArea || !m_isConnecting) return;
    
    m_currentPos = currentPos;
    updatePreviewConnector(drawArea);
    
    // 检查是否悬停在目标流程图元素上
    FlowchartBaseItem* targetItem = findFlowchartItemAt(drawArea, currentPos);
    ConnectionManager* connectionManager = drawArea->getConnectionManager();
    
    if (targetItem && targetItem != m_sourceItem && connectionManager) {
        connectionManager->showConnectionPoints(targetItem);
        
        // 检查是否悬停在连接点上
        ConnectionManager::ConnectionPoint* point = 
            connectionManager->findConnectionPointAt(currentPos, 15.0);
        if (point && !point->isOccupied) {
            connectionManager->highlightConnectionPoint(*point);
            setCursorSmoothly(drawArea, Qt::PointingHandCursor);
        } else {
            connectionManager->clearHighlight();
            setCursorSmoothly(drawArea, Qt::CrossCursor);
        }
    } else if (connectionManager) {
        connectionManager->clearHighlight();
        setCursorSmoothly(drawArea, Qt::CrossCursor);
    }
}

void AutoConnectState::finishConnection(DrawArea* drawArea, const QPointF& endPos)
{
    if (!drawArea || !m_isConnecting || !m_sourceItem) return;
    
    // 查找目标元素和连接点
    FlowchartBaseItem* targetItem = findFlowchartItemAt(drawArea, endPos);
    ConnectionManager* connectionManager = drawArea->getConnectionManager();
    
    if (targetItem && targetItem != m_sourceItem && connectionManager) {
        ConnectionManager::ConnectionPoint* targetPoint = 
            connectionManager->findConnectionPointAt(endPos, 15.0);
            
        if (targetPoint && !targetPoint->isOccupied) {
            // 使用命令系统创建连接，支持撤销重做
            ConnectionCommand* command = new ConnectionCommand(
                connectionManager,
                m_sourceItem, m_sourcePointIndex,
                targetItem, targetPoint->index,
                m_connectorType, m_arrowType
            );
            
            // 通过命令管理器执行命令
            CommandManager& cmdManager = CommandManager::getInstance();
            cmdManager.executeCommand(command);
            
            Logger::info(QString("通过命令系统创建连接: %1 -> %2")
                       .arg(m_sourceItem->getText())
                       .arg(targetItem->getText()));
        }
    }
    
    // 清理状态
    cancelConnection(drawArea);
}

void AutoConnectState::cancelConnection(DrawArea* drawArea)
{
    if (!drawArea) return;
    
    // 移除预览连接器
    removePreviewConnector(drawArea);
    
    // 重置状态和缓存
    m_isConnecting = false;
    m_sourceItem = nullptr;
    m_sourcePointIndex = -1;
    m_lastHoveredItem = nullptr;
    m_lastHoveredPoint = nullptr;
    
    // 隐藏连接点和清除高亮
    ConnectionManager* connectionManager = drawArea->getConnectionManager();
    if (connectionManager) {
        connectionManager->hideConnectionPoints();
        connectionManager->clearHighlight();
    }
    
    // 确保覆盖层也清除
    ConnectionPointOverlay* overlay = drawArea->getConnectionOverlay();
    if (overlay) {
        overlay->clearHighlight();
        overlay->setConnectionPointsVisible(false);
    }
    
    // 恢复光标
    setCursorSmoothly(drawArea, Qt::CrossCursor);
}

FlowchartBaseItem* AutoConnectState::findFlowchartItemAt(DrawArea* drawArea, const QPointF& scenePos)
{
    if (!drawArea || !drawArea->scene()) return nullptr;
    
    // 获取该位置的所有项目
    QList<QGraphicsItem*> items = drawArea->scene()->items(scenePos);
    
    // 查找第一个FlowchartBaseItem，跳过ConnectionPointOverlay
    for (QGraphicsItem* item : items) {
        // 跳过ConnectionPointOverlay
        if (dynamic_cast<ConnectionPointOverlay*>(item)) {
            continue;
        }
        
        FlowchartBaseItem* flowchartItem = dynamic_cast<FlowchartBaseItem*>(item);
        if (flowchartItem) {
            return flowchartItem;
        }
    }
    
    return nullptr;
}

int AutoConnectState::findConnectionPointIndex(FlowchartBaseItem* item, const QPointF& scenePos)
{
    if (!item) return -1;
    
    std::vector<QPointF> points = item->getConnectionPoints();
    const double tolerance = 15.0;
    
    for (int i = 0; i < points.size(); ++i) {
        if (QLineF(scenePos, points[i]).length() <= tolerance) {
            return i;
        }
    }
    
    return -1;
}

void AutoConnectState::createPreviewConnector(DrawArea* drawArea)
{
    if (!drawArea || !drawArea->scene() || m_previewConnector) return;
    
    // 创建预览线条
    m_previewConnector = new QGraphicsLineItem(
        m_startPos.x(), m_startPos.y(),
        m_currentPos.x(), m_currentPos.y()
    );
    
    // 设置预览样式
    QPen pen(Qt::blue, 2);
    pen.setStyle(Qt::DashLine);
    static_cast<QGraphicsLineItem*>(m_previewConnector)->setPen(pen);
    
    drawArea->scene()->addItem(m_previewConnector);
}

void AutoConnectState::updatePreviewConnector(DrawArea* drawArea)
{
    if (!drawArea || !m_previewConnector) return;
    
    // 更新预览线条位置
    static_cast<QGraphicsLineItem*>(m_previewConnector)->setLine(
        m_startPos.x(), m_startPos.y(),
        m_currentPos.x(), m_currentPos.y()
    );
}

void AutoConnectState::removePreviewConnector(DrawArea* drawArea)
{
    if (!drawArea || !drawArea->scene() || !m_previewConnector) return;
    
    drawArea->scene()->removeItem(m_previewConnector);
    delete m_previewConnector;
    m_previewConnector = nullptr;
}

void AutoConnectState::setCursorSmoothly(DrawArea* drawArea, Qt::CursorShape shape)
{
    if (!drawArea) return;
    
    // 只有在光标形状真正改变时才更新，避免频繁设置
    if (m_currentCursor != shape) {
        m_currentCursor = shape;
        drawArea->setCursor(QCursor(shape));
    }
} 