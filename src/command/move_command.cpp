#include "move_command.h"

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

QString MoveCommand::getDescription() const {
    if (!m_graphic) return "移动图形";
    
    QString graphicType = Graphic::graphicTypeToString(m_graphic->getType());
    return QString("移动%1 (%2, %3)").arg(graphicType)
            .arg(m_offset.x()).arg(m_offset.y());
}

QString MoveCommand::getType() const {
    return "transform";
}