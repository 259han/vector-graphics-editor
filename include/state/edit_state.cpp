#include "../../include/state/edit_state.h"
#include "../../include/ui/draw_area.h"
#include "../../include/command/move_command.h"
#include <QMouseEvent>

void EditState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event) {
    m_selectedGraphic = drawArea->getGraphicManager().getGraphicAtPosition(event->pos());
    
    if (m_selectedGraphic) {
        m_isMoving = true;
        m_lastPos = event->pos();
    }
}

void EditState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (m_isMoving && m_selectedGraphic) {
        QPointF offset = event->pos() - m_lastPos;
        
        auto moveCommand = std::make_unique<MoveCommand>(m_selectedGraphic, offset);
        moveCommand->execute();
        
        m_isMoving = false;
        m_selectedGraphic = nullptr;
        drawArea->update();
    }
}

void EditState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (m_isMoving && m_selectedGraphic) {
        QPointF offset = event->pos() - m_lastPos;
        m_selectedGraphic->move(offset);
        m_lastPos = event->pos();
        drawArea->update();
    }
}

void EditState::paintEvent(DrawArea* drawArea, QPainter* painter) {
    if (m_selectedGraphic) {
        // 高亮选中的图形
        QRectF boundingBox = m_selectedGraphic->getBoundingBox();
        painter->setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter->drawRect(boundingBox);
    }
}