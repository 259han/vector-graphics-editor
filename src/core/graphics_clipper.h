#ifndef GRAPHICS_CLIPPER_H
#define GRAPHICS_CLIPPER_H

#include <QGraphicsItem>
#include <QPointF>
#include <QPainterPath>
#include <QList>

class DrawArea;
class GraphicItem;

// 图形裁剪器类，负责处理图形的裁剪操作
class GraphicsClipper {
public:
    GraphicsClipper();
    ~GraphicsClipper();
    
    // 使用矩形路径对图形项进行裁剪
    QList<QGraphicsItem*> clipItemsWithRect(DrawArea* drawArea, const QRectF& rect);
    
    // 使用任意路径对图形项进行裁剪
    QList<QGraphicsItem*> clipItemsWithPath(DrawArea* drawArea, const QPainterPath& clipPath);
    
    // 移除之前创建的所有裁剪项
    void removeClippedItems();
    
    // 获取所有裁剪后的图形项
    QList<QGraphicsItem*> getClippedItems() const { return m_clippedItems; }
    
    // 获取选区内的裁剪项（被标记为selected的项）
    QList<QGraphicsItem*> getSelectedClippedItems() const;

private:
    // 裁剪后的图形项列表
    QList<QGraphicsItem*> m_clippedItems;
    
    // 创建裁剪后的图形项
    QGraphicsItem* createClippedItem(DrawArea* drawArea, QGraphicsItem* sourceItem, 
                                    const QPainterPath& clipPath, bool isInside);
};

#endif // GRAPHICS_CLIPPER_H 