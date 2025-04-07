#include "rotate_command.h"

RotateCommand::RotateCommand(Graphic* graphic, double angle)
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