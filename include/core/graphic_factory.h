#ifndef GRAPHIC_FACTORY_H
#define GRAPHIC_FACTORY_H

#include "graphic.h"
#include <memory>
#include <vector>

class GraphicFactory {
public:
    virtual std::unique_ptr<Graphic> createGraphic(Graphic::GraphicType type) = 0;
    virtual std::unique_ptr<Graphic> createCustomGraphic(Graphic::GraphicType type, const std::vector<QPointF>& points) = 0;
    
    virtual ~GraphicFactory() = default;
};

class DefaultGraphicFactory : public GraphicFactory {
public:
    std::unique_ptr<Graphic> createGraphic(Graphic::GraphicType type) override;
    std::unique_ptr<Graphic> createCustomGraphic(Graphic::GraphicType type, const std::vector<QPointF>& points) override;
};

#endif // GRAPHIC_FACTORY_H