#ifndef ROTATE_COMMAND_H
#define ROTATE_COMMAND_H

#include "command.h"
#include "../core/graphic_item.h"

class RotateCommand : public Command {
public:
    RotateCommand(GraphicItem* graphic, double angle);
    
    void execute() override;
    void undo() override;
    QString getDescription() const override;
    QString getType() const override;

private:
    GraphicItem* m_graphic;
    double m_angle;
};

#endif // ROTATE_COMMAND_H