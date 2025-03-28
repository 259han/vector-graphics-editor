#ifndef EDITOR_STATE_H
#define EDITOR_STATE_H

#include <QMouseEvent>
#include <QPainter>

class DrawArea;

class EditorState {
public:
    virtual ~EditorState() = default;
    
    virtual void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
    virtual void paintEvent(DrawArea* drawArea, QPainter* painter) = 0;
};

#endif // EDITOR_STATE_H