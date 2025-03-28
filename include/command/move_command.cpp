#include "../../include/command/move_command.h"

MoveCommand::MoveCommand(Graphic* graphic, const QPointF& offset)
    : m_graphic(graphic), m_offset(offset) {}

void MoveCommand::execute() {
    if (m_graphic) {
        m_graphic->move(m_offset);
    }
}

void MoveCommand::undo() {
    if (m_graphic) {
        m_graphic->move(-m_offset);
    }
}