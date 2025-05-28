#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <QMouseEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QPointF>
#include <QString>
#include <QCursor>
#include <QMainWindow>
#include <QDebug>
#include <QWheelEvent>

class DrawArea;

/**
 * @brief 编辑器状态的基类，实现状态模式
 * 
 * 提供编辑器状态的基本接口和通用实现，所有具体状态都应该继承此类
 */
class EditorState {
public:
    // 状态类型枚举
    enum StateType {
        BaseState,
        DrawState,
        EditState,
        FillState,
        ClipState,
        AutoConnectState
    };
    
    EditorState();
    virtual ~EditorState() = default;
    
    // 事件处理接口，必须由子类实现
    virtual void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void paintEvent(DrawArea* drawArea, QPainter* painter) = 0;
    virtual void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) = 0;
    
    // 可选实现的事件处理
    virtual void keyReleaseEvent(DrawArea* drawArea, QKeyEvent* event) {}
    virtual void wheelEvent(DrawArea* drawArea, QWheelEvent* event) {}
    virtual void enterEvent(DrawArea* drawArea, QEvent* event) {}
    virtual void leaveEvent(DrawArea* drawArea, QEvent* event) {}
    
    // 处理鼠标左右键点击的虚函数
    virtual void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) {}
    virtual void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) {}
    virtual void handleMiddleMousePress(DrawArea* drawArea, QPointF scenePos) {}
    
    // 状态切换通知
    virtual void onEnterState(DrawArea* drawArea) {}
    virtual void onExitState(DrawArea* drawArea) {}
    
    // 状态类型检查
    virtual StateType getStateType() const = 0;
    bool isEditState() const { return getStateType() == EditState; }
    bool isDrawState() const { return getStateType() == DrawState; }
    bool isFillState() const { return getStateType() == FillState; }
    bool isClipState() const { return getStateType() == ClipState; }
    
    // 状态名称
    virtual QString getStateName() const = 0;
    
protected:
    // 更新状态栏消息的通用方法
    void updateStatusMessage(DrawArea* drawArea, const QString& message);
    
    // 设置光标的通用方法
    void setCursor(DrawArea* drawArea, const QCursor& cursor);
    
    // 重置为默认光标
    void resetCursor(DrawArea* drawArea);
    
    // 通用场景点获取方法
    QPointF getScenePos(DrawArea* drawArea, QMouseEvent* event);
    
    // 获取场景点与网格对齐
    QPointF getSnapToGridPoint(DrawArea* drawArea, const QPointF& scenePos);
    
    // 通用的退出当前状态方法 (通常用于Escape键处理)
    void exitCurrentState(DrawArea* drawArea);
    
    // 通用日志输出方法
    void logDebug(const QString& message);
    void logInfo(const QString& message);
    void logWarning(const QString& message);
    void logError(const QString& message);
    
    // 通用的检查右键点击并切换到编辑状态
    bool checkRightClickToEdit(DrawArea* drawArea, QMouseEvent* event);
    
    // 处理常见的键盘快捷键
    bool handleCommonKeyboardShortcuts(DrawArea* drawArea, QKeyEvent* event);
    
    // 鼠标状态跟踪
    bool m_isPressed = false;
    QPointF m_lastPoint;
    
    // 缩放和平移处理
    bool handleZoomAndPan(DrawArea* drawArea, QWheelEvent* event);
    
    // 设置提示工具提示
    void setToolTip(DrawArea* drawArea, const QString& tip);
};

#endif // EDITOR_STATE_H