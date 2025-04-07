#ifndef EDIT_STATE_H
#define EDIT_STATE_H

#include "editor_state.h"
#include "../core/graphic_item.h"
#include "../core/selection_manager.h"
#include "../core/graphics_clipper.h"
#include "../command/selection_command.h"
#include <QPointF>
#include <QRectF>
#include <vector>
#include <QCursor>
#include <QGraphicsRectItem>

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
    
    // 处理鼠标左右键点击
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) override;
    
    // 状态判断
    bool isEditState() const override { return true; }

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
    
    // 选择区域管理器
    SelectionManager* m_selectionManager = nullptr;
    
    // 图形裁剪器
    GraphicsClipper* m_graphicsClipper = nullptr;
    
    // 更新鼠标样式
    void updateCursor(GraphicItem::ControlHandle handle);
    
    // 图形变换处理
    void handleItemRotation(DrawArea* drawArea, const QPointF& pos, GraphicItem* item);
    void handleItemScaling(DrawArea* drawArea, const QPointF& pos, GraphicItem* item);
    
    // 创建变换命令
    SelectionCommand* createMoveCommand(DrawArea* drawArea, const QPointF& offset);
    SelectionCommand* createDeleteCommand(DrawArea* drawArea);
    SelectionCommand* createClipCommand(DrawArea* drawArea);
};

#endif // EDIT_STATE_H