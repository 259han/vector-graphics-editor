#ifndef AUTO_CONNECT_STATE_H
#define AUTO_CONNECT_STATE_H

#include "editor_state.h"
#include "../core/flowchart_base_item.h"
#include "../core/connection_manager.h"
#include "../core/flowchart_connector_item.h"
#include <QPointF>
#include <QGraphicsItem>
#include <QTimer>

/**
 * @brief 自动连接状态类
 * 
 * 用于在流程图元素之间创建自动连接。当用户从一个流程图元素拖拽到另一个时，
 * 自动创建连接线
 */
class AutoConnectState : public EditorState {

public:
    explicit AutoConnectState();
    ~AutoConnectState() override;

    // EditorState接口实现
    StateType getStateType() const override { return EditorState::AutoConnectState; }
    QString getStateName() const override { return "AutoConnect"; }
    void onEnterState(DrawArea* drawArea) override;
    void onExitState(DrawArea* drawArea) override;
    
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) override;
    
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) override;

    // 设置连接器样式
    void setConnectorType(FlowchartConnectorItem::ConnectorType type);
    void setArrowType(FlowchartConnectorItem::ArrowType type);

private:
    // 状态管理
    bool m_isConnecting;                        // 是否正在连接过程中
    FlowchartBaseItem* m_sourceItem;           // 源流程图元素
    int m_sourcePointIndex;                    // 源连接点索引
    QPointF m_startPos;                        // 起始位置
    QPointF m_currentPos;                      // 当前位置
    
    // 预览连接器
    QGraphicsItem* m_previewConnector;         // 预览连接线
    
    // 连接器样式
    FlowchartConnectorItem::ConnectorType m_connectorType;
    FlowchartConnectorItem::ArrowType m_arrowType;
    
    // 缓存和优化相关
    FlowchartBaseItem* m_lastHoveredItem;      // 上次悬停的流程图元素
    ConnectionManager::ConnectionPoint* m_lastHoveredPoint; // 上次悬停的连接点
    Qt::CursorShape m_currentCursor;           // 当前光标状态
    QTimer* m_hoverUpdateTimer;                // 悬停更新定时器
    
    // 内部方法
    void startConnection(DrawArea* drawArea, FlowchartBaseItem* sourceItem, int pointIndex, const QPointF& startPos);
    void updateConnectionPreview(DrawArea* drawArea, const QPointF& currentPos);
    void finishConnection(DrawArea* drawArea, const QPointF& endPos);
    void cancelConnection(DrawArea* drawArea);
    
    FlowchartBaseItem* findFlowchartItemAt(DrawArea* drawArea, const QPointF& scenePos);
    int findConnectionPointIndex(FlowchartBaseItem* item, const QPointF& scenePos);
    
    void createPreviewConnector(DrawArea* drawArea);
    void updatePreviewConnector(DrawArea* drawArea);
    void removePreviewConnector(DrawArea* drawArea);
    
    // 优化方法
    void setCursorSmoothly(DrawArea* drawArea, Qt::CursorShape cursor);
};

#endif // AUTO_CONNECT_STATE_H 