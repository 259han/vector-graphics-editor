#ifndef EDIT_STATE_H
#define EDIT_STATE_H

#include "editor_state.h"
#include "../../include/core/graphic.h"

class EditState : public EditorState {
public:
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;

private:
    Graphic* m_selectedGraphic = nullptr;
    QPointF m_lastPos;
    bool m_isMoving = false;
};

#endif // EDIT_STATE_H