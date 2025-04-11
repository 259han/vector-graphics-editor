#include "core/selection_manager.h"
#include "ui/draw_area.h"
#include "core/graphic_item.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QWidget>
#include <QDebug>

SelectionManager::SelectionManager(QGraphicsScene* scene)
    : QObject(nullptr)
    , m_scene(scene)
    , m_selectionRect(nullptr)
    , m_isDraggingSelection(false)
    , m_selectionMode(SingleSelection)
    , m_filter(nullptr)
{
    // 创建选择区域矩形，但不添加到场景中
    m_selectionRect = new QGraphicsRectItem();
    m_selectionRect->setZValue(1000); // 确保选择矩形显示在其他项的顶部
    updateSelectionAppearance();
}

SelectionManager::~SelectionManager()
{
    try {
        qDebug() << "SelectionManager: 析构开始";
        
        // 断开所有信号连接
        this->disconnect();
        
        // 安全地清除选择，不触发信号
        m_selectedItems.clear();
        
        // 安全检查：确保m_selectionRect有效并且在场景中
        if (m_selectionRect) {
            if (m_selectionRect->scene() && m_scene && m_selectionRect->scene() == m_scene) {
                m_scene->removeItem(m_selectionRect);
            }
            // 删除选择矩形
            delete m_selectionRect;
            m_selectionRect = nullptr;
        }
        
        // 清空场景引用
        m_scene = nullptr;
        
        qDebug() << "SelectionManager: 析构完成";
    }
    catch (const std::exception& e) {
        qCritical() << "SelectionManager::~SelectionManager: 异常:" << e.what();
    }
    catch (...) {
        qCritical() << "SelectionManager::~SelectionManager: 未知异常";
    }
}

void SelectionManager::setScene(QGraphicsScene* scene)
{
    // 如果选择矩形在旧场景中，移除它
    if (m_selectionRect && m_selectionRect->scene()) {
        m_selectionRect->scene()->removeItem(m_selectionRect);
    }
    
    m_scene = scene;
    
    // 清除当前选择
    m_selectedItems.clear();
    
    // 发出选择改变信号
    emit selectionChanged();
}

void SelectionManager::startSelection(const QPointF& startPoint, SelectionMode mode)
{
    m_startPoint = startPoint;
    m_currentPoint = startPoint;
    m_selectionMode = mode;
    
    // 确保选择矩形已从场景中移除
    if (m_selectionRect->scene()) {
        m_selectionRect->scene()->removeItem(m_selectionRect);
    }
    
    // 设置初始大小为0
    m_selectionRect->setRect(QRectF(m_startPoint, QSizeF(0, 0)));
    
    // 添加到场景
    if (m_scene) {
        m_scene->addItem(m_selectionRect);
    }
    
    // 在多选模式下，保存之前的选择
    if (m_selectionMode == MultiSelection) {
        m_previousSelection = m_selectedItems;
    } else if (m_selectionMode == SingleSelection) {
        // 在单选模式下，清除之前的选择
        m_selectedItems.clear();
    }
    
    // 发出选择开始信号
    emit selectionStarted();
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
        // 根据选择区域更新选择的项
        updateSelectionFromRect();
    }
    
    // 如果选择矩形在场景中，移除它
    if (m_selectionRect && m_selectionRect->scene()) {
        m_selectionRect->scene()->removeItem(m_selectionRect);
    }
    
    // 应用选择到场景
    applySelectionToScene();
    
    // 发出选择完成信号
    emit selectionFinished();
    emit selectionChanged();
}

void SelectionManager::updateSelectionFromRect()
{
    // 如果没有场景或选择矩形无效，返回
    if (!m_scene || !isSelectionValid()) {
        return;
    }
    
    // 获取选择区域内的所有图形项
    QList<QGraphicsItem*> itemsInRect = m_scene->items(getSelectionRect());
    
    // 如果是单选模式，清除之前的选择
    if (m_selectionMode == SingleSelection) {
        m_selectedItems.clear();
    }
    
    // 处理每个选择区域内的项
    for (QGraphicsItem* item : itemsInRect) {
        // 跳过选择矩形自身
        if (item == m_selectionRect) {
            continue;
        }
        
        // 应用过滤器
        if (!applyFilter(item)) {
            continue;
        }
        
        // 在多选模式下，如果项已经在之前的选择中，保持其选中状态
        if (m_selectionMode == MultiSelection && m_previousSelection.contains(item)) {
            continue;
        }
        
        // 添加到选择集合
        m_selectedItems.insert(item);
    }
}

bool SelectionManager::applyFilter(QGraphicsItem* item) const
{
    // 如果没有设置过滤器，则接受所有项
    if (!m_filter) {
        return true;
    }
    
    // 应用过滤器
    return m_filter(item);
}

void SelectionManager::clearSelection()
{
    try {
        qDebug() << "SelectionManager::clearSelection: 开始清除选择";
        
        // 检查对象有效性
        if (!m_scene) {
            qDebug() << "SelectionManager::clearSelection: 场景为空，无需清除";
            m_selectedItems.clear();
            m_isDraggingSelection = false;
            return;
        }
        
        // 如果选择矩形在场景中，移除它
        if (m_selectionRect && m_selectionRect->scene()) {
            qDebug() << "SelectionManager::clearSelection: 移除选择矩形";
            // 安全检查：确保m_selectionRect->scene()是m_scene
            if (m_selectionRect->scene() == m_scene) {
                m_scene->removeItem(m_selectionRect);
            }
        }
        
        // 记录被清除的项目数量
        int itemCount = m_selectedItems.size();
        qDebug() << "SelectionManager::clearSelection: 清除" << itemCount << "个选中项";
        
        // 安全清除当前选择
        QSet<QGraphicsItem*> itemsToDeselect;
        for (QGraphicsItem* item : m_selectedItems) {
            if (item && m_scene->items().contains(item)) {
                itemsToDeselect.insert(item);
            }
        }
        
        // 取消选中有效的项目
        for (QGraphicsItem* item : itemsToDeselect) {
            if (item) {
                item->setSelected(false);
            }
        }
        
        // 清除选择集合
        m_selectedItems.clear();
        
        // 应用选择到场景
        if (m_scene) {
            applySelectionToScene();
        }
        
        // 重置拖动状态
        m_isDraggingSelection = false;
        
        // 发出选择改变信号
        emit selectionChanged();
        
        qDebug() << "SelectionManager::clearSelection: 选择已清除";
    }
    catch (const std::exception& e) {
        qCritical() << "SelectionManager::clearSelection: 异常:" << e.what();
        
        // 即使出错，也要确保清除状态
        m_selectedItems.clear();
        m_isDraggingSelection = false;
    }
    catch (...) {
        qCritical() << "SelectionManager::clearSelection: 未知异常";
        
        // 即使出错，也要确保清除状态
        m_selectedItems.clear();
        m_isDraggingSelection = false;
    }
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
    // 移动所有选中的图形项
    for (QGraphicsItem* item : m_selectedItems) {
        item->moveBy(offset.x(), offset.y());
    }
    
    // 发出选择改变信号
    emit selectionChanged();
}

QList<QGraphicsItem*> SelectionManager::getSelectedItems() const
{
    // 将QSet转换为QList，使用QList构造函数而不是toList方法
    return QList<QGraphicsItem*>(m_selectedItems.begin(), m_selectedItems.end());
}

GraphicItem::ControlHandle SelectionManager::handleAtPoint(const QPointF& point) const
{
    if (!m_selectionRect || !isSelectionValid() || m_selectedItems.isEmpty()) {
        return GraphicItem::None;
    }
    
    // 获取选择矩形的边界矩形（场景坐标系）
    QRectF rect = m_selectionRect->rect();
    if (!m_selectionRect->scene()) {
        return GraphicItem::None;
    }
    
    // 使用与 GraphicItem 相同的控制点大小
    const int handleSize = GraphicItem::getHandleSize();
    
    // 将场景坐标转换为选择矩形的本地坐标
    QPointF localPoint = m_selectionRect->mapFromScene(point);
    QRectF localRect = m_selectionRect->boundingRect();
    
    // 检查各个控制点
    // 左上角
    if (QRectF(localRect.left() - handleSize/2, localRect.top() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到左上角控制点";
        return GraphicItem::TopLeft;
    }
    // 右上角
    else if (QRectF(localRect.right() - handleSize/2, localRect.top() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到右上角控制点";
        return GraphicItem::TopRight;
    }
    // 左下角
    else if (QRectF(localRect.left() - handleSize/2, localRect.bottom() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到左下角控制点";
        return GraphicItem::BottomLeft;
    }
    // 右下角
    else if (QRectF(localRect.right() - handleSize/2, localRect.bottom() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到右下角控制点";
        return GraphicItem::BottomRight;
    }
    // 上中
    else if (QRectF(localRect.center().x() - handleSize/2, localRect.top() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到上中控制点";
        return GraphicItem::TopCenter;
    }
    // 下中
    else if (QRectF(localRect.center().x() - handleSize/2, localRect.bottom() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到下中控制点";
        return GraphicItem::BottomCenter;
    }
    // 左中
    else if (QRectF(localRect.left() - handleSize/2, localRect.center().y() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到左中控制点";
        return GraphicItem::MiddleLeft;
    }
    // 右中
    else if (QRectF(localRect.right() - handleSize/2, localRect.center().y() - handleSize/2, handleSize, handleSize).contains(localPoint)) {
        qDebug() << "SelectionManager: 检测到右中控制点";
        return GraphicItem::MiddleRight;
    }
    
    return GraphicItem::None;
}

void SelectionManager::scaleSelection(GraphicItem::ControlHandle handle, const QPointF& point)
{
    // 如果没有选中项，返回
    if (m_selectedItems.isEmpty()) {
        qDebug() << "SelectionManager::scaleSelection: 没有选中项，无法缩放";
        return;
    }
    
    qDebug() << "SelectionManager::scaleSelection: 尝试缩放选择，控制点类型:" << handle << "位置:" << point;
    
    // 获取选中项的边界矩形
    QRectF boundingRect;
    bool first = true;
    for (QGraphicsItem* item : m_selectedItems) {
        if (!item) continue;
        
        // 使用sceneBoundingRect确保获取图形项在场景中的精确边界
        QRectF itemRect = item->sceneBoundingRect();
        
        if (first) {
            boundingRect = itemRect;
            first = false;
        } else {
            boundingRect = boundingRect.united(itemRect);
        }
    }
    
    // 计算缩放因子
    qreal scaleX = 1.0;
    qreal scaleY = 1.0;
    QPointF center = boundingRect.center();
    
    // 根据控制点类型计算适当的缩放因子
    switch (handle) {
        case GraphicItem::TopLeft:
            scaleX = (boundingRect.right() - point.x()) / boundingRect.width();
            scaleY = (boundingRect.bottom() - point.y()) / boundingRect.height();
            break;
        case GraphicItem::TopRight:
            scaleX = (point.x() - boundingRect.left()) / boundingRect.width();
            scaleY = (boundingRect.bottom() - point.y()) / boundingRect.height();
            break;
        case GraphicItem::BottomLeft:
            scaleX = (boundingRect.right() - point.x()) / boundingRect.width();
            scaleY = (point.y() - boundingRect.top()) / boundingRect.height();
            break;
        case GraphicItem::BottomRight:
            scaleX = (point.x() - boundingRect.left()) / boundingRect.width();
            scaleY = (point.y() - boundingRect.top()) / boundingRect.height();
            break;
        case GraphicItem::TopCenter:
            // 只改变Y方向的缩放，X方向保持不变
            scaleX = 1.0;
            scaleY = (boundingRect.bottom() - point.y()) / boundingRect.height();
            break;
        case GraphicItem::BottomCenter:
            // 只改变Y方向的缩放，X方向保持不变
            scaleX = 1.0;
            scaleY = (point.y() - boundingRect.top()) / boundingRect.height();
            break;
        case GraphicItem::MiddleLeft:
            // 只改变X方向的缩放，Y方向保持不变
            scaleX = (boundingRect.right() - point.x()) / boundingRect.width();
            scaleY = 1.0;
            break;
        case GraphicItem::MiddleRight:
            // 只改变X方向的缩放，Y方向保持不变
            scaleX = (point.x() - boundingRect.left()) / boundingRect.width();
            scaleY = 1.0;
            break;
        default:
            qDebug() << "SelectionManager::scaleSelection: 无效的控制点类型:" << handle;
            return;
    }
    
    // 防止缩放因子过小或无效
    if (scaleX <= 0.1) scaleX = 0.1;
    if (scaleY <= 0.1) scaleY = 0.1;
    
    qDebug() << "SelectionManager::scaleSelection: 计算的缩放因子 X:" << scaleX << "Y:" << scaleY;
    
    // 应用缩放到所有选中项
    for (QGraphicsItem* item : m_selectedItems) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (graphicItem) {
            // 获取当前缩放
            QPointF currentScale = graphicItem->getScale();
            // 应用新缩放
            QPointF newScale(currentScale.x() * scaleX, currentScale.y() * scaleY);
            qDebug() << "SelectionManager::scaleSelection: 图形项旧缩放:" << currentScale << "新缩放:" << newScale;
            graphicItem->setScale(newScale);
        } else {
            qDebug() << "SelectionManager::scaleSelection: 图形项转换失败，无法应用缩放";
        }
    }
    
    // 更新选择矩形
    if (m_selectionRect && m_selectionRect->scene()) {
        // 重新计算选中项的边界矩形
        QRectF newBoundingRect;
        bool first = true;
        for (QGraphicsItem* item : m_selectedItems) {
            if (!item) continue;
            
            // 使用精确的边界矩形计算
            QRectF itemRect = item->sceneBoundingRect();
            
            if (first) {
                newBoundingRect = itemRect;
                first = false;
            } else {
                newBoundingRect = newBoundingRect.united(itemRect);
            }
        }
        m_selectionRect->setRect(newBoundingRect);
    } else {
        qDebug() << "SelectionManager::scaleSelection: 选择矩形不存在或不在场景中，无法更新选择区域";
    }
    
    // 发出选择改变信号
    emit selectionChanged();
}

void SelectionManager::paint(QPainter* painter, const QGraphicsView* view)
{
    // 如果没有选中项，返回
    if (m_selectedItems.isEmpty()) {
        return;
    }
    
    painter->save();
    
    // 计算选中项的边界矩形
    QRectF boundingRect;
    bool first = true;
    for (QGraphicsItem* item : m_selectedItems) {
        if (!item) continue;
        
        // 使用sceneBoundingRect确保获取图形项在场景中的精确边界
        // 这对椭圆等特殊图形尤为重要
        QRectF itemRect = item->sceneBoundingRect();
        
        if (first) {
            boundingRect = itemRect;
            first = false;
        } else {
            boundingRect = boundingRect.united(itemRect);
        }
    }
    
    // 设置控制点样式
    painter->setPen(QPen(Qt::blue, 1));
    painter->setBrush(QBrush(Qt::white));
    
    // 控制点大小
    const int handleSize = 8;
    
    // 现在我们可以使用QGraphicsView的mapFromScene方法
    if (view) {
        // 将场景坐标的矩形转换为视图坐标的多个点
        QPointF topLeft = view->mapFromScene(boundingRect.topLeft());
        QPointF topRight = view->mapFromScene(boundingRect.topRight());
        QPointF bottomLeft = view->mapFromScene(boundingRect.bottomLeft());
        QPointF bottomRight = view->mapFromScene(boundingRect.bottomRight());
        QPointF center = view->mapFromScene(boundingRect.center());
        
        // 绘制8个控制点
        // 左上
        painter->drawRect(QRectF(topLeft.x() - handleSize/2, topLeft.y() - handleSize/2, 
                         handleSize, handleSize));
        
        // 上中
        painter->drawRect(QRectF(center.x() - handleSize/2, topLeft.y() - handleSize/2,
                         handleSize, handleSize));
        
        // 右上
        painter->drawRect(QRectF(topRight.x() - handleSize/2, topRight.y() - handleSize/2,
                         handleSize, handleSize));
        
        // 中左
        painter->drawRect(QRectF(topLeft.x() - handleSize/2, center.y() - handleSize/2,
                         handleSize, handleSize));
        
        // 中右
        painter->drawRect(QRectF(topRight.x() - handleSize/2, center.y() - handleSize/2,
                         handleSize, handleSize));
        
        // 左下
        painter->drawRect(QRectF(bottomLeft.x() - handleSize/2, bottomLeft.y() - handleSize/2,
                         handleSize, handleSize));
        
        // 下中
        painter->drawRect(QRectF(center.x() - handleSize/2, bottomLeft.y() - handleSize/2,
                         handleSize, handleSize));
        
        // 右下
        painter->drawRect(QRectF(bottomRight.x() - handleSize/2, bottomRight.y() - handleSize/2,
                         handleSize, handleSize));
        
        // 绘制选择边框
        painter->setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
        painter->setBrush(Qt::transparent);
        painter->drawPolygon(QPolygonF() << topLeft << topRight << bottomRight << bottomLeft);
    } else {
        // 如果view为空，直接在场景坐标中绘制
        QRectF rect = boundingRect;
        
        // 直接绘制控制点（在场景坐标系中）
        // 左上
        painter->drawRect(QRectF(rect.left() - handleSize/2, rect.top() - handleSize/2, 
                         handleSize, handleSize));
        
        // 上中
        painter->drawRect(QRectF(rect.center().x() - handleSize/2, rect.top() - handleSize/2,
                         handleSize, handleSize));
        
        // 右上
        painter->drawRect(QRectF(rect.right() - handleSize/2, rect.top() - handleSize/2,
                         handleSize, handleSize));
        
        // 中左
        painter->drawRect(QRectF(rect.left() - handleSize/2, rect.center().y() - handleSize/2,
                         handleSize, handleSize));
        
        // 中右
        painter->drawRect(QRectF(rect.right() - handleSize/2, rect.center().y() - handleSize/2,
                         handleSize, handleSize));
        
        // 左下
        painter->drawRect(QRectF(rect.left() - handleSize/2, rect.bottom() - handleSize/2,
                         handleSize, handleSize));
        
        // 下中
        painter->drawRect(QRectF(rect.center().x() - handleSize/2, rect.bottom() - handleSize/2,
                         handleSize, handleSize));
        
        // 右下
        painter->drawRect(QRectF(rect.right() - handleSize/2, rect.bottom() - handleSize/2,
                         handleSize, handleSize));
        
        // 绘制选择边框
        painter->setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
        painter->setBrush(Qt::transparent);
        painter->drawRect(rect);
    }
    
    painter->restore();
}

void SelectionManager::addToSelection(QGraphicsItem* item)
{
    // 如果项有效且通过过滤器
    if (item && applyFilter(item)) {
        m_selectedItems.insert(item);
        applySelectionToScene();
        emit selectionChanged();
    }
}

void SelectionManager::removeFromSelection(QGraphicsItem* item)
{
    // 如果项在选择中
    if (m_selectedItems.contains(item)) {
        m_selectedItems.remove(item);
        applySelectionToScene();
        emit selectionChanged();
    }
}

void SelectionManager::toggleSelection(QGraphicsItem* item)
{
    // 如果项已选中，取消选择；否则，添加到选择
    if (m_selectedItems.contains(item)) {
        m_selectedItems.remove(item);
    } else if (applyFilter(item)) {
        m_selectedItems.insert(item);
    }
    
    applySelectionToScene();
    emit selectionChanged();
}

bool SelectionManager::isSelected(QGraphicsItem* item) const
{
    return m_selectedItems.contains(item);
}

QPointF SelectionManager::selectionCenter() const
{
    // 计算选中项的中心点
    if (m_selectedItems.isEmpty()) {
        return QPointF();
    }
    
    QRectF boundingRect;
    bool first = true;
    for (QGraphicsItem* item : m_selectedItems) {
        if (!item) continue;
        
        // 使用精确的边界矩形计算
        QRectF itemRect = item->sceneBoundingRect();
        
        if (first) {
            boundingRect = itemRect;
            first = false;
        } else {
            boundingRect = boundingRect.united(itemRect);
        }
    }
    
    return boundingRect.center();
}

void SelectionManager::applySelectionToScene()
{
    // 如果没有场景，返回
    if (!m_scene) {
        qWarning() << "SelectionManager::applySelectionToScene: 场景为空";
        return;
    }
    
    try {
        // 先清除所有项的选择状态
        QList<QGraphicsItem*> items = m_scene->items();
        for (QGraphicsItem* item : items) {
            if (item) {
                item->setSelected(false);
            }
        }
        
        // 设置选中项的选择状态，检查项是否仍然有效
        QSet<QGraphicsItem*> invalidItems;
        for (QGraphicsItem* item : m_selectedItems) {
            if (!item) {
                qWarning() << "SelectionManager::applySelectionToScene: 尝试选择空项";
                invalidItems.insert(item);
                continue;
            }
            
            // 检查项是否仍在场景中
            if (item->scene() != m_scene) {
                qWarning() << "SelectionManager::applySelectionToScene: 项不在当前场景中";
                invalidItems.insert(item);
                continue;
            }
            
            // 确保调用setSelected不会导致崩溃
            try {
                item->setSelected(true);
            } catch (const std::exception& e) {
                qWarning() << "SelectionManager::applySelectionToScene: 设置项选中状态时异常:" << e.what();
                invalidItems.insert(item);
            } catch (...) {
                qWarning() << "SelectionManager::applySelectionToScene: 设置项选中状态时未知异常";
                invalidItems.insert(item);
            }
        }
        
        // 从选择集合中移除无效项
        for (QGraphicsItem* item : invalidItems) {
            m_selectedItems.remove(item);
        }
    }
    catch (const std::exception& e) {
        qCritical() << "SelectionManager::applySelectionToScene: 异常:" << e.what();
    }
    catch (...) {
        qCritical() << "SelectionManager::applySelectionToScene: 未知异常";
    }
}

void SelectionManager::syncSelectionFromScene()
{
    try {
        qDebug() << "SelectionManager::syncSelectionFromScene: 开始从场景同步选择";
        
        // 如果没有场景，返回
        if (!m_scene) {
            qWarning() << "SelectionManager::syncSelectionFromScene: 场景为空";
            return;
        }
        
        // 清除当前选择
        int oldCount = m_selectedItems.size();
        m_selectedItems.clear();
        
        // 将场景中所有选中的项添加到选择中
        QList<QGraphicsItem*> sceneSelectedItems = m_scene->selectedItems();
        int newCount = 0;
        
        for (QGraphicsItem* item : sceneSelectedItems) {
            if (!item) {
                qWarning() << "SelectionManager::syncSelectionFromScene: 遇到空项";
                continue;
            }
            
            if (applyFilter(item)) {
                m_selectedItems.insert(item);
                newCount++;
            }
        }
        
        qDebug() << "SelectionManager::syncSelectionFromScene: 从" << oldCount << "项更新为" << newCount << "项";
        
        // 发出选择改变信号
        emit selectionChanged();
        
        qDebug() << "SelectionManager::syncSelectionFromScene: 完成同步";
    }
    catch (const std::exception& e) {
        qCritical() << "SelectionManager::syncSelectionFromScene: 异常:" << e.what();
    }
    catch (...) {
        qCritical() << "SelectionManager::syncSelectionFromScene: 未知异常";
    }
} 