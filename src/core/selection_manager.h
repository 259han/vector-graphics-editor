#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <QGraphicsItem>
#include <QPointF>
#include <QPainterPath>
#include <QList>
#include <QGraphicsRectItem>
#include "../core/graphic_item.h"

class DrawArea;
class GraphicItem;

// 选择区域管理器类，负责处理选择区域的创建、显示和交互
class SelectionManager {
public:
    SelectionManager(DrawArea* drawArea);
    ~SelectionManager();
    
    // 开始创建选择区域
    void startSelection(const QPointF& startPoint);
    
    // 更新选择区域
    void updateSelection(const QPointF& currentPoint);
    
    // 完成选择操作
    void finishSelection();
    
    // 清除选择区域
    void clearSelection();
    
    // 获取选择区域
    QRectF getSelectionRect() const;
    
    // 获取选择区域路径
    QPainterPath getSelectionPath() const;
    
    // 检查选择区域是否有效
    bool isSelectionValid() const;
    
    // 设置选择区域的外观
    void setSelectionAppearance(const QPen& pen, const QBrush& brush);
    
    // 检查点是否在选择区域内
    bool contains(const QPointF& point) const;
    
    // 处理选择区域的移动
    void moveSelection(const QPointF& offset);
    
    // 获取选择区域中的所有图形项
    QList<QGraphicsItem*> getSelectedItems() const;
    
    // 设置是否正在拖动选择区域
    void setDraggingSelection(bool dragging) { m_isDraggingSelection = dragging; }
    
    // 判断是否正在拖动选择区域
    bool isDraggingSelection() const { return m_isDraggingSelection; }
    
    // 设置绘图区域
    void setDrawArea(DrawArea* drawArea) { m_drawArea = drawArea; }
    
    // 获取绘图区域
    DrawArea* getDrawArea() const { return m_drawArea; }
    
    // 检查点是否在控制点上，如果是则返回控制点类型
    GraphicItem::ControlHandle handleAtPoint(const QPointF& point) const;
    
    // 缩放选择区域
    void scaleSelection(GraphicItem::ControlHandle handle, const QPointF& point);
    
    // 绘制选择区域和控制点
    void paintEvent(DrawArea* drawArea, QPainter* painter);

private:
    DrawArea* m_drawArea;
    QGraphicsRectItem* m_selectionRect;
    QPointF m_startPoint;
    QPointF m_currentPoint;
    bool m_isDraggingSelection;
    
    // 更新选择区域的外观
    void updateSelectionAppearance();
};

#endif // SELECTION_MANAGER_H 