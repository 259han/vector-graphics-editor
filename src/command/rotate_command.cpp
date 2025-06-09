#include "rotate_command.h"

RotateCommand::RotateCommand(GraphicItem* graphic, double angle)
    : m_graphic(graphic), m_angle(angle) {}

void RotateCommand::execute() {
    if (m_graphic) {
        m_graphic->rotate(m_angle);
    }
}

void RotateCommand::undo() {
    if (m_graphic) {
        m_graphic->rotate(-m_angle);
    }
}

QString RotateCommand::getDescription() const {
    if (!m_graphic) return "旋转图形";
    
    QString graphicType = GraphicItem::graphicTypeToString(m_graphic->getType());
    return QString("旋转%1 (%2°)").arg(graphicType).arg(m_angle);
}

QString RotateCommand::getType() const {
    return "transform";
}