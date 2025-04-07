#ifndef SCALE_COMMAND_H
#define SCALE_COMMAND_H

#include "command.h"
#include "../core/graphic.h"

class ScaleCommand : public Command {
public:
    ScaleCommand(Graphic* graphic, double factor);
    
    void execute() override;
    void undo() override;
    QString getDescription() const override;
    QString getType() const override;

private:
    Graphic* m_graphic;
    double m_scaleFactor;
};

#endif // SCALE_COMMAND_H