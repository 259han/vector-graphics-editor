#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <QMouseEvent>
#include <QPainter>
#include <QKeyEvent>
#include <QPointF>

class DrawArea;

class EditorState {
public:
    virtual ~EditorState() = default;
    
    virtual void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void paintEvent(DrawArea* drawArea, QPainter* painter) = 0;
    virtual void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) = 0;
    
    // 处理鼠标左右键点击的虚函数
    virtual void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) {}
    virtual void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) {}
    
    virtual bool isEditState() const = 0;
};

#endif // EDITOR_STATE_H