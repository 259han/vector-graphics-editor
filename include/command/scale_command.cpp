#include "../../include/command/scale_command.h"

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