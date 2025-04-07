#ifndef MOVE_COMMAND_H
#define MOVE_COMMAND_H

#include "command.h"
#include "../core/graphic.h"
#include <QPointF>

class MoveCommand : public Command {
public:
    MoveCommand(Graphic* graphic, const QPointF& offset);
    
    void execute() override;
    void undo() override;
    
    QString getDescription() const override;
    QString getType() const override;

private:
    Graphic* m_graphic;
    QPointF m_offset;
};

#endif // MOVE_COMMAND_H