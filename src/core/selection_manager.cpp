#include "core/selection_manager.h"
#include "ui/draw_area.h"
#include "core/graphic_item.h"
#include <QGraphicsScene>
#include <QPainter>

SelectionManager::SelectionManager(DrawArea* drawArea)
    : m_drawArea(drawArea)
    , m_selectionRect(nullptr)
    , m_isDraggingSelection(false)
{
    // 创建选择区域矩形，但不添加到场景中
    m_selectionRect = new QGraphicsRectItem();
    m_selectionRect->setZValue(1000); // 确保选择矩形显示在其他项的顶部
    updateSelectionAppearance();
}

SelectionManager::~SelectionManager()
{
    clearSelection();
    delete m_selectionRect;
}

void SelectionManager::startSelection(const QPointF& startPoint)
{
    m_startPoint = startPoint;
    m_currentPoint = startPoint;
    
    // 确保选择矩形已从场景中移除
    if (m_selectionRect->scene()) {
        m_selectionRect->scene()->removeItem(m_selectionRect);
    }
    
    // 设置初始大小为0
    m_selectionRect->setRect(QRectF(m_startPoint, QSizeF(0, 0)));
    
    // 添加到场景
    if (m_drawArea->scene()) {
        m_drawArea->scene()->addItem(m_selectionRect);
    }
}

void SelectionManager::updateSelection(const QPointF& currentPoint)
{
    m_currentPoint = currentPoint;
    
    // 计算选择区域
    qreal left = qMin(m_startPoint.x(), m_currentPoint.x());
    qreal top = qMin(m_startPoint.y(), m_currentPoint.y());
    qreal width = qAbs(m_currentPoint.x() - m_startPoint.x());
    qreal height = qAbs(m_currentPoint.y() - m_startPoint.y());
    
    // 更新选择矩形的位置和大小
    m_selectionRect->setRect(QRectF(left, top, width, height));
}

void SelectionManager::finishSelection()
{
    // 如果选择区域有效
    if (isSelectionValid()) {
        // 选择区域内的所有图形项
        QList<QGraphicsItem*> items = m_drawArea->scene()->items(getSelectionRect());
        
        // 清除之前的选择状态
        for (QGraphicsItem* item : m_drawArea->scene()->items()) {
            item->setSelected(false);
        }
        
        // 设置选择状态
        for (QGraphicsItem* item : items) {
            GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
            if (graphicItem && item != m_selectionRect) {
                graphicItem->setSelected(true);
            }
        }
    }
}

void SelectionManager::clearSelection()
{
    // 如果选择矩形在场景中，移除它
    if (m_selectionRect && m_selectionRect->scene()) {
        m_selectionRect->scene()->removeItem(m_selectionRect);
    }
    
    // 清除所有项的选择状态
    if (m_drawArea && m_drawArea->scene()) {
        for (QGraphicsItem* item : m_drawArea->scene()->items()) {
            item->setSelected(false);
        }
    }
    
    m_isDraggingSelection = false;
}

QRectF SelectionManager::getSelectionRect() const
{
    return m_selectionRect ? m_selectionRect->rect() : QRectF();
}

QPainterPath SelectionManager::getSelectionPath() const
{
    QPainterPath path;
    if (m_selectionRect) {
        path.addRect(m_selectionRect->rect());
    }
    return path;
}

bool SelectionManager::isSelectionValid() const
{
    if (!m_selectionRect) {
        return false;
    }
    
    QRectF rect = m_selectionRect->rect();
    return rect.width() > 5 && rect.height() > 5; // 确保选择区域足够大
}

void SelectionManager::setSelectionAppearance(const QPen& pen, const QBrush& brush)
{
    if (m_selectionRect) {
        m_selectionRect->setPen(pen);
        m_selectionRect->setBrush(brush);
    }
}

void SelectionManager::updateSelectionAppearance()
{
    if (m_selectionRect) {
        // 设置选择矩形的外观
        QPen pen(Qt::DashLine);
        pen.setColor(QColor(0, 120, 215));
        pen.setWidth(1);
        
        QBrush brush(QColor(0, 120, 215, 40)); // 半透明填充
        
        m_selectionRect->setPen(pen);
        m_selectionRect->setBrush(brush);
    }
}

bool SelectionManager::contains(const QPointF& point) const
{
    return m_selectionRect && m_selectionRect->rect().contains(point);
}

void SelectionManager::moveSelection(const QPointF& offset)
{
    if (m_selectionRect) {
        // 移动选择矩形
        m_selectionRect->moveBy(offset.x(), offset.y());
        
        // 移动所有选中的图形项
        QList<QGraphicsItem*> selectedItems = getSelectedItems();
        for (QGraphicsItem* item : selectedItems) {
            item->moveBy(offset.x(), offset.y());
        }
    }
}

QList<QGraphicsItem*> SelectionManager::getSelectedItems() const
{
    QList<QGraphicsItem*> selectedItems;
    
    if (m_drawArea && m_drawArea->scene()) {
        for (QGraphicsItem* item : m_drawArea->scene()->items()) {
            if (item->isSelected() && item != m_selectionRect) {
                selectedItems.append(item);
            }
        }
    }
    
    return selectedItems;
}

// 新增方法实现

GraphicItem::ControlHandle SelectionManager::handleAtPoint(const QPointF& point) const
{
    if (!m_selectionRect || !isSelectionValid()) {
        return GraphicItem::None;
    }
    
    // 获取选择矩形的边界矩形（场景坐标系）
    QRectF rect = m_selectionRect->rect();
    if (!m_selectionRect->scene()) {
        return GraphicItem::None;
    }
    
    // 控制点大小
    const int handleSize = 8;
    
    // 检查各个控制点
    // 左上角
    if (QRectF(rect.left() - handleSize/2, rect.top() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::TopLeft;
    }
    // 右上角
    else if (QRectF(rect.right() - handleSize/2, rect.top() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::TopRight;
    }
    // 左下角
    else if (QRectF(rect.left() - handleSize/2, rect.bottom() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::BottomLeft;
    }
    // 右下角
    else if (QRectF(rect.right() - handleSize/2, rect.bottom() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::BottomRight;
    }
    // 上中
    else if (QRectF(rect.center().x() - handleSize/2, rect.top() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::TopCenter;
    }
    // 下中
    else if (QRectF(rect.center().x() - handleSize/2, rect.bottom() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::BottomCenter;
    }
    // 左中
    else if (QRectF(rect.left() - handleSize/2, rect.center().y() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::MiddleLeft;
    }
    // 右中
    else if (QRectF(rect.right() - handleSize/2, rect.center().y() - handleSize/2, handleSize, handleSize).contains(point)) {
        return GraphicItem::MiddleRight;
    }
    
    return GraphicItem::None;
}

void SelectionManager::scaleSelection(GraphicItem::ControlHandle handle, const QPointF& point)
{
    if (!m_selectionRect || !isSelectionValid()) {
        return;
    }
    
    QRectF rect = m_selectionRect->rect();
    QPointF topLeft = rect.topLeft();
    QPointF bottomRight = rect.bottomRight();
    
    // 根据操作手柄调整矩形的尺寸
    switch (handle) {
        case GraphicItem::TopLeft:
            rect.setTopLeft(point);
            break;
        case GraphicItem::TopRight:
            rect.setTopRight(point);
            break;
        case GraphicItem::BottomLeft:
            rect.setBottomLeft(point);
            break;
        case GraphicItem::BottomRight:
            rect.setBottomRight(point);
            break;
        case GraphicItem::TopCenter:
            rect.setTop(point.y());
            break;
        case GraphicItem::BottomCenter:
            rect.setBottom(point.y());
            break;
        case GraphicItem::MiddleLeft:
            rect.setLeft(point.x());
            break;
        case GraphicItem::MiddleRight:
            rect.setRight(point.x());
            break;
        default:
            break;
    }
    
    // 归一化矩形，确保宽高为正
    rect = rect.normalized();
    
    // 更新选择矩形
    m_selectionRect->setRect(rect);
    
    // 如果有选中的图形项，也需要相应调整它们的大小
    // 这部分比较复杂，需要根据项目的具体实现来调整
    // 暂时省略
}

void SelectionManager::paintEvent(DrawArea* drawArea, QPainter* painter)
{
    // 如果存在选择区域，绘制其控制点
    if (m_selectionRect && isSelectionValid() && m_selectionRect->scene()) {
        painter->save();
        
        // 转换到视图坐标
        QRectF sceneRect = m_selectionRect->rect();
        QRect viewRect = drawArea->mapFromScene(sceneRect).boundingRect();
        
        // 设置控制点样式
        painter->setPen(QPen(Qt::blue, 1));
        painter->setBrush(QBrush(Qt::white));
        
        // 控制点大小
        const int handleSize = 8;
        
        // 绘制8个控制点
        // 左上
        painter->drawRect(QRect(viewRect.left() - handleSize/2, viewRect.top() - handleSize/2, 
                         handleSize, handleSize));
        
        // 上中
        painter->drawRect(QRect(viewRect.center().x() - handleSize/2, viewRect.top() - handleSize/2,
                         handleSize, handleSize));
        
        // 右上
        painter->drawRect(QRect(viewRect.right() - handleSize/2, viewRect.top() - handleSize/2,
                         handleSize, handleSize));
        
        // 中左
        painter->drawRect(QRect(viewRect.left() - handleSize/2, viewRect.center().y() - handleSize/2,
                         handleSize, handleSize));
        
        // 中右
        painter->drawRect(QRect(viewRect.right() - handleSize/2, viewRect.center().y() - handleSize/2,
                         handleSize, handleSize));
        
        // 左下
        painter->drawRect(QRect(viewRect.left() - handleSize/2, viewRect.bottom() - handleSize/2,
                         handleSize, handleSize));
        
        // 下中
        painter->drawRect(QRect(viewRect.center().x() - handleSize/2, viewRect.bottom() - handleSize/2,
                         handleSize, handleSize));
        
        // 右下
        painter->drawRect(QRect(viewRect.right() - handleSize/2, viewRect.bottom() - handleSize/2,
                         handleSize, handleSize));
        
        painter->restore();
    }
} 