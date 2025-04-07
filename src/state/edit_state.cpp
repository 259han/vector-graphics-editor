#include "state/edit_state.h"
#include "ui/draw_area.h"
#include "core/graphic_item.h"
#include "core/selection_manager.h"
#include "core/graphics_clipper.h"
#include "command/selection_command.h"
#include "command/command_manager.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPen>
#include <QApplication>

EditState::EditState()
    : m_isAreaSelecting(false),
      m_selectionStart(),
      m_isDragging(false),
      m_dragStartPosition(),
      m_activeHandle(GraphicItem::None),
      m_isRotating(false),
      m_isScaling(false),
      m_initialAngle(0.0),
      m_transformOrigin(),
      m_selectionManager(nullptr),
      m_graphicsClipper(nullptr)
{
    qDebug() << "Edit State: Created";
    
    // 创建选择区域管理器和图形裁剪器
    m_selectionManager = new SelectionManager(nullptr);
    m_graphicsClipper = new GraphicsClipper();
}

EditState::~EditState()
{
    // 清理资源
    delete m_selectionManager;
    delete m_graphicsClipper;
}

void EditState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    qDebug() << "Edit State: 左键点击，位置:" << scenePos;
    
    // 确保选择管理器使用正确的绘图区域
    if (m_selectionManager->getDrawArea() != drawArea) {
        m_selectionManager->setDrawArea(drawArea);
    }
    
    // 检查点击位置是否有图形项或选择区域
    QGraphicsScene* scene = drawArea->scene();
    QGraphicsItem* item = scene->itemAt(scenePos, drawArea->transform());
    
    // 检查点击是否在选择区域上
    if (m_selectionManager->isSelectionValid() && 
        (m_selectionManager->contains(scenePos) || m_selectionManager->handleAtPoint(scenePos) != GraphicItem::None)) {
        
        // 获取选择区域上的控制点
        m_activeHandle = m_selectionManager->handleAtPoint(scenePos);
        
        if (m_activeHandle != GraphicItem::None) {
            // 如果点击在控制点上，设置缩放状态
            m_isScaling = true;
            m_transformOrigin = scenePos;
            updateCursor(m_activeHandle);
            qDebug() << "Edit State: Starting selection area scaling with handle:" << m_activeHandle;
            return;
        } else {
            // 点击在选择区域上，但不在控制点上，准备拖动
            m_dragStartPosition = QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
            m_isDragging = true;
            m_selectionManager->setDraggingSelection(true);
            qDebug() << "Edit State: Preparing to drag selection area";
            return;
        }
    }
    
    // 如果点击了某个普通图形项
    if (item) {
        if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
            // 查看是否点击了图形项的控制点
            m_activeHandle = graphicItem->handleAtPoint(scenePos);
            
            if (m_activeHandle != GraphicItem::None) {
                // 如果点击在控制点上，设置变换状态
                if (m_activeHandle == GraphicItem::Rotation) {
                    // 旋转控制点
                    m_isRotating = true;
                    m_transformOrigin = graphicItem->pos();
                    m_initialAngle = atan2(scenePos.y() - m_transformOrigin.y(), 
                                          scenePos.x() - m_transformOrigin.x());
                } else {
                    // 缩放控制点
                    m_isScaling = true;
                    m_transformOrigin = graphicItem->pos();
                    m_initialDistance = sqrt(pow(scenePos.x() - m_transformOrigin.x(), 2) + 
                                           pow(scenePos.y() - m_transformOrigin.y(), 2));
                }
                
                // 设置相应的光标
                updateCursor(m_activeHandle);
            } else {
                // 点击在图形项本身上，选中它
                scene->clearSelection();
                item->setSelected(true);
                
                // 准备拖动
                m_isDragging = true;
                m_dragStartPosition = QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
                
                // 创建选择管理器选择
                QList<QGraphicsItem*> selectedItems = scene->selectedItems();
                if (!selectedItems.isEmpty()) {
                    QRectF selectionRect;
                    for (QGraphicsItem* selectedItem : selectedItems) {
                        selectionRect = selectionRect.united(selectedItem->boundingRect().translated(selectedItem->pos()));
                    }
                    
                    m_selectionManager->startSelection(selectionRect.topLeft());
                    m_selectionManager->updateSelection(selectionRect.bottomRight());
                    m_selectionManager->finishSelection();
                    m_selectionManager->setDraggingSelection(true);
                }
            }
        } else {
            // 如果点击的不是GraphicItem，则考虑开始选区
            scene->clearSelection();
            m_selectionManager->clearSelection();
            
            // 开始区域选择
            m_isAreaSelecting = true;
            m_selectionStart = QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
            m_selectionManager->startSelection(scenePos);
        }
    } else {
        // 点击空白区域，开始选区或清除选择
        scene->clearSelection();
        m_selectionManager->clearSelection();
        
        // 开始区域选择
        m_isAreaSelecting = true;
        m_selectionStart = QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
        m_selectionManager->startSelection(scenePos);
    }
}

void EditState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    qDebug() << "Edit State: 右键点击，位置:" << scenePos;
    
    // 右键点击清除选择
    m_selectionManager->clearSelection();
    drawArea->scene()->clearSelection();
}

void EditState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event)
{
    // 获取场景坐标
    QPointF scenePos = drawArea->mapToScene(event->pos());
    
    if (event->button() == Qt::LeftButton) {
        handleLeftMousePress(drawArea, scenePos);
    } else if (event->button() == Qt::RightButton) {
        handleRightMousePress(drawArea, scenePos);
    }
    
    // 接受此事件
    event->accept();
}

void EditState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event)
{
    // 获取场景坐标
    QPointF scenePos = drawArea->mapToScene(event->pos());
    
    // 如果正在进行区域选择
    if (m_isAreaSelecting) {
        // 更新选择区域
        m_selectionManager->updateSelection(scenePos);
        return;
    }
    
    // 如果正在旋转图形项
    if (m_isRotating) {
        // 获取选中的图形项
        QGraphicsItem* item = nullptr;
        QList<QGraphicsItem*> selectedItems = drawArea->scene()->selectedItems();
        if (!selectedItems.isEmpty()) {
            item = selectedItems.first();
            if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
                handleItemRotation(drawArea, scenePos, graphicItem);
            }
        }
        return;
    }
    
    // 如果正在缩放图形项
    if (m_isScaling) {
        // 获取选中的图形项
        QGraphicsItem* item = nullptr;
        QList<QGraphicsItem*> selectedItems = drawArea->scene()->selectedItems();
        if (!selectedItems.isEmpty()) {
            item = selectedItems.first();
            if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
                handleItemScaling(drawArea, scenePos, graphicItem);
            }
        } else if (m_selectionManager->isSelectionValid()) {
            // 如果没有选中的图形项但有选择区域，则缩放选择区域
            m_selectionManager->scaleSelection(m_activeHandle, scenePos);
        }
        return;
    }
    
    // 如果正在拖动选择区域或图形项
    if (m_isDragging) {
        // 计算拖动的位移量
        QPointF viewDelta = event->pos() - m_dragStartPosition;
        QPointF sceneDelta = drawArea->mapToScene(event->pos()) - drawArea->mapToScene(m_dragStartPosition);
        
        if (m_selectionManager->isDraggingSelection()) {
            // 移动选择区域
            m_selectionManager->moveSelection(sceneDelta);
        } else {
            // 移动选中的图形项
            QList<QGraphicsItem*> selectedItems = drawArea->scene()->selectedItems();
            for (QGraphicsItem* item : selectedItems) {
                item->moveBy(sceneDelta.x(), sceneDelta.y());
            }
        }
        
        // 更新拖动起始位置
        m_dragStartPosition = event->pos();
        return;
    }
    
    // 如果没有执行任何拖动/选择操作，检查鼠标是否在控制点上以更新鼠标形状
    QGraphicsScene* scene = drawArea->scene();
    QGraphicsItem* item = scene->itemAt(scenePos, drawArea->transform());
    
    if (item) {
        // 检查是否在选择区域的控制点上
        if (m_selectionManager->isSelectionValid()) {
            GraphicItem::ControlHandle handle = m_selectionManager->handleAtPoint(scenePos);
            if (handle != GraphicItem::None) {
                updateCursor(handle);
                return;
            }
        }
        
        // 检查是否在图形项的控制点上
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (graphicItem && graphicItem->isSelected()) {
            QPointF itemPos = graphicItem->mapFromScene(scenePos);
            GraphicItem::ControlHandle handle = graphicItem->handleAtPoint(itemPos);
            if (handle != GraphicItem::None) {
                updateCursor(handle);
                return;
            }
        }
    }
    
    // 如果不在任何控制点上，恢复默认光标
    QApplication::restoreOverrideCursor();
}

void EditState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event)
{
    // 获取场景坐标
    QPointF scenePos = drawArea->mapToScene(event->pos());
    
    // 如果正在进行区域选择
    if (m_isAreaSelecting) {
        // 完成选择区域
        m_selectionManager->finishSelection();
        m_isAreaSelecting = false;
        return;
    }
    
    // 如果正在拖动选择区域或图形项
    if (m_isDragging) {
        // 如果是选择区域
        if (m_selectionManager->isDraggingSelection()) {
            // 创建移动命令并执行
            SelectionCommand* moveCmd = createMoveCommand(drawArea, QPointF(0, 0)); // 最后的位移已经应用了
            if (moveCmd) {
                // 这里应该将命令添加到命令管理器中
                // CommandManager::getInstance()->executeCommand(moveCmd);
                delete moveCmd; // 暂时直接删除，实际应该由命令管理器管理
            }
            
            m_selectionManager->setDraggingSelection(false);
        } else {
            // 创建移动命令并执行
            QList<QGraphicsItem*> selectedItems = drawArea->scene()->selectedItems();
            if (!selectedItems.isEmpty()) {
                // 这里需要计算总的位移量，暂时省略
                // SelectionCommand* moveCmd = createMoveCommand(drawArea, totalDelta);
                // CommandManager::getInstance()->executeCommand(moveCmd);
            }
        }
        
        m_isDragging = false;
        return;
    }
    
    // 如果正在旋转或缩放
    if (m_isRotating || m_isScaling) {
        // 这里可以添加旋转或缩放完成后的命令
        // 暂时省略
        
        m_isRotating = false;
        m_isScaling = false;
        m_activeHandle = GraphicItem::None;
        
        // 恢复默认光标
        QApplication::restoreOverrideCursor();
        return;
    }
}

void EditState::keyPressEvent(DrawArea* drawArea, QKeyEvent* event)
{
    // 处理Delete键 - 删除选中的图形项
    if (event->key() == Qt::Key_Delete) {
        QList<QGraphicsItem*> selectedItems = drawArea->scene()->selectedItems();
        if (!selectedItems.isEmpty()) {
            // 创建删除命令并执行
            SelectionCommand* deleteCmd = createDeleteCommand(drawArea);
            if (deleteCmd) {
                // CommandManager::getInstance()->executeCommand(deleteCmd);
                delete deleteCmd; // 暂时直接删除，实际应该由命令管理器管理
            }
        }
        event->accept();
    }
    // 处理Escape键 - 清除选择
    else if (event->key() == Qt::Key_Escape) {
        m_selectionManager->clearSelection();
        drawArea->scene()->clearSelection();
        event->accept();
    }
    // 处理剪切、复制、粘贴
    else if (event->key() == Qt::Key_C && event->modifiers() & Qt::ControlModifier) {
        // Ctrl+C: 复制
        drawArea->copySelectedItems();
        event->accept();
    }
    else if (event->key() == Qt::Key_V && event->modifiers() & Qt::ControlModifier) {
        // Ctrl+V: 粘贴
        drawArea->pasteItems();
        event->accept();
    }
    else if (event->key() == Qt::Key_X && event->modifiers() & Qt::ControlModifier) {
        // Ctrl+X: 剪切
        drawArea->cutSelectedItems();
        event->accept();
    }
}

void EditState::paintEvent(DrawArea* drawArea, QPainter* painter)
{
    // 由选择管理器绘制选择区域和控制点
    m_selectionManager->paintEvent(drawArea, painter);
}

// 更新鼠标样式
void EditState::updateCursor(GraphicItem::ControlHandle handle)
{
    switch (handle) {
        case GraphicItem::TopLeft:
        case GraphicItem::BottomRight:
            QApplication::setOverrideCursor(Qt::SizeFDiagCursor);
            break;
        case GraphicItem::TopRight:
        case GraphicItem::BottomLeft:
            QApplication::setOverrideCursor(Qt::SizeBDiagCursor);
            break;
        case GraphicItem::TopCenter:
        case GraphicItem::BottomCenter:
            QApplication::setOverrideCursor(Qt::SizeVerCursor);
            break;
        case GraphicItem::MiddleLeft:
        case GraphicItem::MiddleRight:
            QApplication::setOverrideCursor(Qt::SizeHorCursor);
            break;
        case GraphicItem::Rotation:
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            break;
        default:
            QApplication::restoreOverrideCursor();
            break;
    }
}

// 处理图形项旋转
void EditState::handleItemRotation(DrawArea* drawArea, const QPointF& pos, GraphicItem* item)
{
    if (!item) return;
    
    // 计算当前角度
    QPointF centerPos = item->mapToScene(item->boundingRect().center());
    double currentAngle = atan2(pos.y() - centerPos.y(), pos.x() - centerPos.x());
    
    // 计算角度差
    double angleDiff = currentAngle - m_initialAngle;
    double degrees = angleDiff * 180.0 / M_PI;
    
    // 如果按住Shift键，限制为15度的倍数
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        int steps = qRound(degrees / 15.0);
        degrees = steps * 15.0;
    }
    
    // 应用旋转
    item->setRotation(item->rotation() + degrees);
    
    // 更新初始角度
    m_initialAngle = currentAngle;
}

// 处理图形项缩放
void EditState::handleItemScaling(DrawArea* drawArea, const QPointF& pos, GraphicItem* item)
{
    if (!item) return;
    
    // 获取项目的边界矩形
    QRectF rect = item->boundingRect();
    QPointF center = item->mapToScene(rect.center());
    
    // 根据控制点类型进行不同的缩放操作
    QPointF newScale = item->getScale();
    QPointF lastPos = m_transformOrigin;
    m_transformOrigin = pos;
    
    // 计算缩放因子
    QPointF delta = pos - lastPos;
    
    switch (m_activeHandle) {
        case GraphicItem::TopLeft:
            newScale.setX(newScale.x() - delta.x() * 0.01);
            newScale.setY(newScale.y() - delta.y() * 0.01);
            break;
        case GraphicItem::TopCenter:
            newScale.setY(newScale.y() - delta.y() * 0.01);
            break;
        case GraphicItem::TopRight:
            newScale.setX(newScale.x() + delta.x() * 0.01);
            newScale.setY(newScale.y() - delta.y() * 0.01);
            break;
        case GraphicItem::MiddleLeft:
            newScale.setX(newScale.x() - delta.x() * 0.01);
            break;
        case GraphicItem::MiddleRight:
            newScale.setX(newScale.x() + delta.x() * 0.01);
            break;
        case GraphicItem::BottomLeft:
            newScale.setX(newScale.x() - delta.x() * 0.01);
            newScale.setY(newScale.y() + delta.y() * 0.01);
            break;
        case GraphicItem::BottomCenter:
            newScale.setY(newScale.y() + delta.y() * 0.01);
            break;
        case GraphicItem::BottomRight:
            newScale.setX(newScale.x() + delta.x() * 0.01);
            newScale.setY(newScale.y() + delta.y() * 0.01);
            break;
        default:
            break;
    }
    
    // 确保比例不会太小
    const qreal minScale = 0.1;
    newScale.setX(qMax(minScale, newScale.x()));
    newScale.setY(qMax(minScale, newScale.y()));
    
    // 应用新的比例
    if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
        graphicItem->setScale(newScale);
    } else {
        // For regular QGraphicsItem, apply uniform scaling (average of x and y)
        qreal uniformScale = (newScale.x() + newScale.y()) / 2.0;
        item->setScale(uniformScale);
    }
}

// 创建移动命令
SelectionCommand* EditState::createMoveCommand(DrawArea* drawArea, const QPointF& offset)
{
    QList<QGraphicsItem*> selectedItems;
    
    if (m_selectionManager->isDraggingSelection()) {
        selectedItems = m_selectionManager->getSelectedItems();
    } else {
        selectedItems = drawArea->scene()->selectedItems();
    }
    
    if (selectedItems.isEmpty()) {
        return nullptr;
    }
    
    SelectionCommand* moveCmd = new SelectionCommand(drawArea, SelectionCommand::MoveSelection);
    moveCmd->setMoveInfo(selectedItems, offset);
    
    return moveCmd;
}

// 创建删除命令
SelectionCommand* EditState::createDeleteCommand(DrawArea* drawArea)
{
    QList<QGraphicsItem*> selectedItems = drawArea->scene()->selectedItems();
    
    if (selectedItems.isEmpty()) {
        return nullptr;
    }
    
    SelectionCommand* deleteCmd = new SelectionCommand(drawArea, SelectionCommand::DeleteSelection);
    deleteCmd->setDeleteInfo(selectedItems);
    
    return deleteCmd;
}

// 创建裁剪命令
SelectionCommand* EditState::createClipCommand(DrawArea* drawArea)
{
    if (!m_selectionManager->isSelectionValid()) {
        return nullptr;
    }
    
    QRectF selectionRect = m_selectionManager->getSelectionRect();
    QList<QGraphicsItem*> originalItems = drawArea->scene()->items(selectionRect);
    
    // 过滤掉非图形项
    QList<QGraphicsItem*> graphicItems;
    for (QGraphicsItem* item : originalItems) {
        if (dynamic_cast<GraphicItem*>(item)) {
            graphicItems.append(item);
        }
    }
    
    if (graphicItems.isEmpty()) {
        return nullptr;
    }
    
    // 使用裁剪器执行裁剪
    QList<QGraphicsItem*> clippedItems = m_graphicsClipper->clipItemsWithRect(drawArea, selectionRect);
    
    SelectionCommand* clipCmd = new SelectionCommand(drawArea, SelectionCommand::ClipSelection);
    clipCmd->setClipInfo(graphicItems, clippedItems);
    
    return clipCmd;
} 