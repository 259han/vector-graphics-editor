#ifndef ROTATE_COMMAND_H
#define ROTATE_COMMAND_H

#include "command.h"
#include "../core/graphic.h"

class RotateCommand : public Command {
public:
    RotateCommand(Graphic* graphic, double angle);
    
    void execute() override;
    void undo() override;

private:
    Graphic* m_graphic;
    double m_angle;
};

#endif // ROTATE_COMMAND_H