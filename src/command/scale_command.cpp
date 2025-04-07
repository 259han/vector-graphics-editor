#include "scale_command.h"

ScaleCommand::ScaleCommand(Graphic* graphic, double factor)
    : m_graphic(graphic), m_scaleFactor(factor) {}

void ScaleCommand::execute() {
    if (m_graphic) {
        m_graphic->scale(m_scaleFactor);
    }
}

void ScaleCommand::undo() {
    if (m_graphic) {
        m_graphic->scale(1.0 / m_scaleFactor);
    }
}

QString ScaleCommand::getDescription() const {
    if (!m_graphic) return "缩放图形";
    
    QString graphicType = Graphic::graphicTypeToString(m_graphic->getType());
    return QString("缩放%1 (%2x)").arg(graphicType).arg(m_scaleFactor);
}

QString ScaleCommand::getType() const {
    return "transform";
}