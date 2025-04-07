#include "core/graphics_clipper.h"
#include "ui/draw_area.h"
#include "core/graphic_item.h"
#include <QPainter>
#include <QGraphicsScene>

GraphicsClipper::GraphicsClipper()
{
}

GraphicsClipper::~GraphicsClipper()
{
    removeClippedItems();
}

QList<QGraphicsItem*> GraphicsClipper::clipItemsWithRect(DrawArea* drawArea, const QRectF& rect)
{
    QPainterPath clipPath;
    clipPath.addRect(rect);
    return clipItemsWithPath(drawArea, clipPath);
}

QList<QGraphicsItem*> GraphicsClipper::clipItemsWithPath(DrawArea* drawArea, const QPainterPath& clipPath)
{
    if (!drawArea)
        return QList<QGraphicsItem*>();

    // 清除之前的裁剪项
    removeClippedItems();
    
    QGraphicsScene* scene = drawArea->scene();
    QList<QGraphicsItem*> items = scene->items();
    
    for (QGraphicsItem* item : items) {
        // 跳过非GraphicItem类型的项目
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (!graphicItem)
            continue;
            
        // 获取图形项的路径
        QPainterPath itemPath = graphicItem->shape();
        itemPath = graphicItem->mapToScene(itemPath);
        
        // 计算裁剪区域内/外的部分
        QPainterPath insidePath = itemPath & clipPath;
        QPainterPath outsidePath = itemPath - clipPath;
        
        // 如果与裁剪区域有交集，创建新的裁剪项
        if (!insidePath.isEmpty()) {
            QGraphicsItem* insideItem = createClippedItem(drawArea, item, insidePath, true);
            if (insideItem)
                m_clippedItems.append(insideItem);
        }
        
        if (!outsidePath.isEmpty()) {
            QGraphicsItem* outsideItem = createClippedItem(drawArea, item, outsidePath, false);
            if (outsideItem)
                m_clippedItems.append(outsideItem);
        }
    }
    
    return m_clippedItems;
}

QGraphicsItem* GraphicsClipper::createClippedItem(DrawArea* drawArea, QGraphicsItem* sourceItem, 
                                                const QPainterPath& clipPath, bool isInside)
{
    GraphicItem* sourceGraphicItem = dynamic_cast<GraphicItem*>(sourceItem);
    if (!sourceGraphicItem)
        return nullptr;
        
    // 创建一个新的图形项来表示裁剪后的部分
    GraphicItem* newItem = nullptr;
    
    // 这里需要根据原图形项的具体类型创建相应的新图形项
    // 暂时使用简化版本，实际应该根据项目中的具体图形项类型实现
    
    // 应用裁剪路径到新图形项
    // newItem->setPath(clipPath);
    
    // 应用原图形项的样式、颜色等属性
    // newItem->setStyle(sourceGraphicItem->style());
    
    // 如果是选中区域内的部分，标记为selected
    if (isInside)
        newItem->setSelected(true);
    
    // 添加到场景
    if (newItem && drawArea->scene())
        drawArea->scene()->addItem(newItem);
        
    return newItem;
}

void GraphicsClipper::removeClippedItems()
{
    // 从场景中移除所有裁剪项
    for (QGraphicsItem* item : m_clippedItems) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    
    m_clippedItems.clear();
}

QList<QGraphicsItem*> GraphicsClipper::getSelectedClippedItems() const
{
    QList<QGraphicsItem*> selectedItems;
    
    for (QGraphicsItem* item : m_clippedItems) {
        if (item->isSelected()) {
            selectedItems.append(item);
        }
    }
    
    return selectedItems;
} 