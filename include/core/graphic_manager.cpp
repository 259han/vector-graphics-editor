#include "../../include/core/graphic_manager.h"
#include <iostream>

void GraphicManager::addGraphic(std::unique_ptr<Graphic> graphic) {
    m_graphics.push_back(std::move(graphic));
}

void GraphicManager::removeGraphic(size_t index) {
    if (index < m_graphics.size()) {
        m_graphics.erase(m_graphics.begin() + index);
    }
}

void GraphicManager::clearGraphics() {
    std::cout << "Before clearing, current count: " << m_graphics.size() << std::endl;
    m_graphics.clear();
    std::cout << "Graphics cleared. Current count: " << m_graphics.size() << std::endl;
}

std::vector<std::unique_ptr<Graphic>>& GraphicManager::getGraphics() {
    return m_graphics;
}

const std::vector<std::unique_ptr<Graphic>>& GraphicManager::getGraphics() const {
    return m_graphics;
}

Graphic* GraphicManager::getGraphicAtPosition(const QPointF& pos) {
    for (auto& graphic : m_graphics) {
        if (graphic->getBoundingBox().contains(pos)) {
            return graphic.get();
        }
    }
    return nullptr;
}

