#ifndef EDIT_STATE_H
#define EDIT_STATE_H

#include "editor_state.h"
#include "../core/graphic_item.h"
#include "../core/selection_manager.h"
// 裁剪功能已移至future/clip目录
// #include "../core/graphics_clipper.h"
#include "../command/selection_command.h"
#include "../command/style_change_command.h"
#include <QPointF>
#include <QRectF>
#include <vector>
#include <QCursor>
#include <QGraphicsRectItem>
#include <memory>

class DrawArea;

class EditState : public EditorState {
public:
    EditState();
    ~EditState();
    
    // 实现状态接口
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void wheelEvent(DrawArea* drawArea, QWheelEvent* event) override;
    
    // 状态切换通知
    void onEnterState(DrawArea* drawArea) override;
    void onExitState(DrawArea* drawArea) override;
    
    // 处理鼠标左右键点击
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleMiddleMousePress(DrawArea* drawArea, QPointF scenePos) override;
    
    // 样式变更方法
    void applyPenColorChange(const QColor& color);
    void applyPenWidthChange(qreal width);
    void applyBrushColorChange(const QColor& color);
    
    // 状态类型和名称
    StateType getStateType() const override { return StateType::EditState; }
    QString getStateName() const override { return "编辑模式"; }

    // 获取DrawArea的SelectionManager
    SelectionManager* getSelectionManager(DrawArea* drawArea) const;

private:
    // 区域选择相关
    bool m_isAreaSelecting = false;
    QPoint m_selectionStart;
    
    // 拖动相关
    bool m_isDragging = false;
    QPoint m_dragStartPosition;
    
    // 拖拽操作标志
    bool m_isRotating = false;
    bool m_isScaling = false;
    GraphicItem::ControlHandle m_activeHandle = GraphicItem::None;
    
    // 变换参考点
    QPointF m_transformOrigin;
    double m_initialAngle = 0.0;
    double m_initialDistance = 0.0;
    
    DrawArea* m_drawArea = nullptr;
    
    // 更新鼠标样式
    void updateCursor(GraphicItem::ControlHandle handle);
    
    // 图形变换处理
    void handleItemRotation(DrawArea* drawArea, const QPointF& pos, GraphicItem* item);
    void handleItemScaling(DrawArea* drawArea, const QPointF& pos, GraphicItem* item);
    
    // 创建变换命令
    SelectionCommand* createMoveCommand(DrawArea* drawArea, const QPointF& offset);
    SelectionCommand* createDeleteCommand(DrawArea* drawArea);
    
    
    // 创建样式变更命令
    StyleChangeCommand* createStyleChangeCommand(DrawArea* drawArea, 
                                              StyleChangeCommand::StylePropertyType propertyType);
    
    // 新增 m_scaleStartPos 成员变量
    QPointF m_scaleStartPos;
};

#endif // EDIT_STATE_H