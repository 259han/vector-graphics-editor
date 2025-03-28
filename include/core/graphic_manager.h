#ifndef GRAPHIC_MANAGER_H
#define GRAPHIC_MANAGER_H

#include "graphic.h"
#include <vector>
#include <memory>

class GraphicManager {
public:
    void addGraphic(std::unique_ptr<Graphic> graphic);
    void removeGraphic(size_t index);
    void clearGraphics();

    std::vector<std::unique_ptr<Graphic>>& getGraphics();
    const std::vector<std::unique_ptr<Graphic>>& getGraphics() const;

    Graphic* getGraphicAtPosition(const QPointF& pos);


private:
    std::vector<std::unique_ptr<Graphic>> m_graphics;
};

#endif // GRAPHIC_MANAGER_H