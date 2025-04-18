#ifndef GRAPHICS_ITEM_FACTORY_H
#define GRAPHICS_ITEM_FACTORY_H

#include <QGraphicsItem>
#include <memory>
#include "graphic_item.h"

// 创建QGraphicsItem对象的工厂
class GraphicsItemFactory {
public:
    virtual ~GraphicsItemFactory() = default;
    
    // 创建基本图形项
    virtual QGraphicsItem* createItem(GraphicItem::GraphicType type, const QPointF& position) = 0;
    
    // 创建自定义图形项
    virtual QGraphicsItem* createCustomItem(GraphicItem::GraphicType type, const std::vector<QPointF>& points) = 0;
};

// 默认图形项工厂实现
class DefaultGraphicsItemFactory : public GraphicsItemFactory {
public:
    DefaultGraphicsItemFactory() = default;
    ~DefaultGraphicsItemFactory() override = default;
    
    QGraphicsItem* createItem(GraphicItem::GraphicType type, const QPointF& position) override;
    QGraphicsItem* createCustomItem(GraphicItem::GraphicType type, const std::vector<QPointF>& points) override;
};

#endif // GRAPHICS_ITEM_FACTORY_H 