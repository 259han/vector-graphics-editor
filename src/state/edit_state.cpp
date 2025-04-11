#include "edit_state.h"
#include "../ui/draw_area.h"
#include "../core/graphic_item.h"
#include "../core/selection_manager.h"
// 裁剪功能已移至future/clip目录
// #include "../core/graphics_clipper.h"
#include "../command/selection_command.h"
#include "../command/style_change_command.h"
#include "../command/transform_command.h"
#include "../command/command_manager.h"
#include "../utils/logger.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPen>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QTransform>
#include <QGraphicsPathItem>
#include <QtMath>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <cmath>
#include <QStatusBar>
#include <QMainWindow>

EditState::EditState()
    : m_isAreaSelecting(false),
      m_selectionStart(),
      m_isDragging(false),
      m_dragStartPosition(),
      m_isRotating(false),
      m_isScaling(false),
      m_initialAngle(0.0),
      m_transformOrigin(),
      m_scaleStartPos(),
      m_activeHandle(GraphicItem::None)
{
    Logger::info("EditState: 创建编辑状态");
    
    // 裁剪功能已移至future/clip目录
    // m_graphicsClipper = std::make_unique<GraphicsClipper>();
}

EditState::~EditState()
{
    // 智能指针会自动管理资源释放，不需要手动删除
    Logger::info("EditState: 销毁编辑状态");
}

void EditState::onEnterState(DrawArea* drawArea)
{
    if (!drawArea) {
        Logger::warning("EditState::onEnterState: 传入的DrawArea为空");
        return;
    }
    
    Logger::debug("EditState::onEnterState: 开始初始化编辑状态");
    m_drawArea = drawArea;
    
    // 进入编辑状态时禁用画布的拖动功能
    drawArea->setDragMode(QGraphicsView::NoDrag);
    
    // 确保场景上的所有项目可选择
    QGraphicsScene* scene = drawArea->scene();
    if (scene) {
        QList<QGraphicsItem*> items = scene->items();
        int count = 0;
        
        for (QGraphicsItem* item : items) {
            if (item) {
                item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                item->setFlag(QGraphicsItem::ItemIsMovable, true);
                count++;
            }
        }
        
        Logger::debug(QString("EditState::onEnterState: 设置了 %1 个图形项为可选择状态").arg(count));
        
        // 确保场景更新
        scene->update();
    } else {
        Logger::warning("EditState::onEnterState: 场景为空");
    }
    
    // 重置状态变量
    m_isAreaSelecting = false;
    m_isDragging = false;
    m_activeHandle = GraphicItem::None;
    m_isRotating = false;
    m_isScaling = false;
    
    // 确保选择管理器处于正确状态
    SelectionManager* selectionManager = getSelectionManager(drawArea);
    if (selectionManager) {
        // 初始化为单选模式
        selectionManager->setSelectionMode(SelectionManager::SingleSelection);
        selectionManager->setDraggingSelection(false);
        
        // 应用选择状态到场景
        selectionManager->applySelectionToScene();
        
        Logger::debug("EditState::onEnterState: 选择管理器已初始化");
    } else {
        Logger::warning("EditState::onEnterState: 获取选择管理器失败");
    }
    
    // 更新状态信息
    updateStatusMessage(drawArea, "编辑模式：可选择、移动和变换图形");
    
    // 重置鼠标光标
    resetCursor(drawArea);
    
    // 确保视图更新
    drawArea->viewport()->update();
    QApplication::processEvents();
    
    Logger::info("EditState::onEnterState: 编辑状态初始化完成");
}

void EditState::onExitState(DrawArea* drawArea)
{
    if (m_drawArea == drawArea) {
        m_drawArea = nullptr;
    }
    
    // 清除选择
    if (drawArea && drawArea->scene()) {
        drawArea->scene()->clearSelection();
    }
    
    // 更新状态信息
    Logger::info("EditState: 退出编辑状态");
}

void EditState::handleMiddleMousePress(DrawArea* drawArea, QPointF scenePos)
{
    Logger::debug("EditState: 中键点击，位置: " + QString("(%1, %2)").arg(scenePos.x()).arg(scenePos.y()));
    
    // 中键点击可以用于开始平移视图
    if (drawArea) {
        drawArea->setDragMode(QGraphicsView::ScrollHandDrag);
        setCursor(drawArea, Qt::ClosedHandCursor);
    }
}

void EditState::wheelEvent(DrawArea* drawArea, QWheelEvent* event)
{
    // 使用基类的缩放和平移处理
    if (handleZoomAndPan(drawArea, event)) {
        return;
    }
    
    // 如果基类没有处理，则传递给视图的默认处理
    QGraphicsView::ViewportAnchor oldAnchor = drawArea->transformationAnchor();
    drawArea->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    // 缩放因子
    double scaleFactor = 1.15;
    
    if (event->angleDelta().y() > 0) {
        // 放大
        drawArea->scale(scaleFactor, scaleFactor);
    } else {
        // 缩小
        drawArea->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
    
    drawArea->setTransformationAnchor(oldAnchor);
}

void EditState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    // 获取选择管理器
    SelectionManager* selectionManager = getSelectionManager(drawArea);
    if (!selectionManager) {
        return;
    }
    
    // 首先检查是否点击在控制点上
    bool hitControlPoint = false;
    
    // 1. 首先检查选择区域的控制点（优先级最高）
    if (selectionManager->isSelectionValid()) {
        m_activeHandle = selectionManager->handleAtPoint(scenePos);
        if (m_activeHandle != GraphicItem::None) {
            hitControlPoint = true;
            
            // 重置所有状态
            m_isAreaSelecting = false;
            m_isDragging = false; 
            m_isRotating = false;
            
            // 设置缩放状态
            m_isScaling = true;
            m_scaleStartPos = scenePos;
            updateCursor(m_activeHandle);
            return;
        }
    }
    
    // 2. 检查图形项的控制点
    QGraphicsScene* scene = drawArea->scene();
    QGraphicsItem* item = scene->itemAt(scenePos, drawArea->transform());
    
    if (!hitControlPoint && item && dynamic_cast<GraphicItem*>(item)) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        
        // 使用场景坐标，GraphicItem::handleAtPoint内部会做坐标转换
        m_activeHandle = graphicItem->handleAtPoint(scenePos);
        
        if (m_activeHandle != GraphicItem::None) {
            hitControlPoint = true;
            
            // 确保图形项被选中
            if (!selectionManager->isSelected(graphicItem)) {
                selectionManager->clearSelection();
                selectionManager->addToSelection(graphicItem);
            }
            
            // 重置所有状态
            m_isAreaSelecting = false;
            m_isDragging = false;
            
            if (m_activeHandle == GraphicItem::Rotation) {
                m_isRotating = true;
                m_isScaling = false;
                QPointF centerPos = graphicItem->mapToScene(graphicItem->boundingRect().center());
                m_initialAngle = atan2(scenePos.y() - centerPos.y(), scenePos.x() - centerPos.x());
                QApplication::setOverrideCursor(Qt::ClosedHandCursor);
            } else {
                m_isScaling = true;
                m_isRotating = false;
                m_scaleStartPos = scenePos;
                updateCursor(m_activeHandle);
            }
            return;
        }
    }
    
    // 3. 如果没有点击到控制点，则处理普通的选择和拖动
    if (!hitControlPoint) {
        // 重置状态变量
        m_isScaling = false;
        m_isRotating = false;
        m_activeHandle = GraphicItem::None;
        
        if (item) {
            // 根据修饰键处理选择
            if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
                selectionManager->toggleSelection(item);
            } else if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                selectionManager->addToSelection(item);
            } else if (!selectionManager->isSelected(item)) {
                selectionManager->clearSelection();
                selectionManager->addToSelection(item);
            }
            
            // 开始拖动
            m_isAreaSelecting = false;
            m_isScaling = false;
            m_isRotating = false;
            m_isDragging = true;
            selectionManager->setDraggingSelection(true);
            m_dragStartPosition = QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
            QApplication::setOverrideCursor(Qt::SizeAllCursor);
        } else {
            // 点击空白区域，开始区域选择
            selectionManager->clearSelection();
            m_isDragging = false;
            m_isScaling = false;
            m_isRotating = false;
            m_isAreaSelecting = true;
            selectionManager->startSelection(scenePos);
            QApplication::setOverrideCursor(Qt::CrossCursor);
        }
    }
}

void EditState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    qDebug() << "Edit State: 右键点击，位置:" << scenePos;
    
    // 右键点击清除选择
    SelectionManager* selectionManager = getSelectionManager(drawArea);
    if (selectionManager) {
        selectionManager->clearSelection();
    }
    drawArea->scene()->clearSelection();
}

void EditState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = getScenePos(drawArea, event);
        handleLeftMousePress(drawArea, scenePos);
        event->accept();
    } else if (event->button() == Qt::RightButton) {
        QPointF scenePos = getScenePos(drawArea, event);
        handleRightMousePress(drawArea, scenePos);
        event->accept();
    }
}

void EditState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event)
{
    QPointF scenePos = getScenePos(drawArea, event);
    SelectionManager* selectionManager = getSelectionManager(drawArea);
    if (!selectionManager) {
        return;
    }
    
    // 根据当前状态处理鼠标移动
    if (m_isAreaSelecting) {
        // 更新选择区域
        selectionManager->updateSelection(scenePos);
        
        // 确保更新视图
        drawArea->viewport()->update();
    } else if (m_isScaling) {
        // 处理缩放
        if (m_activeHandle != GraphicItem::None) {
            // 如果是单个图形项的缩放
            if (selectionManager->getSelectedItems().size() == 1) {
                QGraphicsItem* item = selectionManager->getSelectedItems().first();
                GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
                if (graphicItem) {
                    handleItemScaling(drawArea, scenePos, graphicItem);
                }
            } else {
                // 使用 SelectionManager 处理多个图形的缩放
                selectionManager->scaleSelection(m_activeHandle, scenePos);
            }
        }
        
        // 确保更新视图
        drawArea->viewport()->update();
    } else if (m_isRotating) {
        // 处理旋转
        if (selectionManager->getSelectedItems().size() == 1) {
            QGraphicsItem* item = selectionManager->getSelectedItems().first();
            GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
            if (graphicItem) {
                handleItemRotation(drawArea, scenePos, graphicItem);
            }
        }
        // 确保更新视图
        drawArea->viewport()->update();
    } else if (m_isDragging) {
        // 移动选中的图形
        QPoint newPos(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
        QPoint delta = newPos - m_dragStartPosition;
        
        // 只有当移动距离超过阈值时才真正移动对象
        if (QPointF(delta).manhattanLength() > 3.0) {
            QPointF deltaF(delta.x(), delta.y());
            drawArea->moveSelectedGraphics(deltaF);
            m_dragStartPosition = newPos;
        }
        
        // 确保更新视图
        drawArea->viewport()->update();
    } else {
        // 未按下鼠标时，更新光标样式
        bool cursorSet = false;
        GraphicItem::ControlHandle handle = GraphicItem::None;
        
        // 1. 首先检查是否在选择区域的控制点上（优先级最高）
        if (selectionManager->isSelectionValid()) {
            handle = selectionManager->handleAtPoint(scenePos);
            if (handle != GraphicItem::None) {
                updateCursor(handle);
                cursorSet = true;
            }
        }
        
        // 2. 如果不在选择区域的控制点上，检查是否在图形项的控制点上
        if (!cursorSet) {
            QGraphicsItem* item = drawArea->scene()->itemAt(scenePos, drawArea->transform());
            if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
                // 直接使用场景坐标传递给handleAtPoint，它会内部处理坐标转换
                handle = graphicItem->handleAtPoint(scenePos);
                if (handle != GraphicItem::None) {
                    updateCursor(handle);
                    cursorSet = true;
                }
                // 3. 如果在图形项上但不在控制点上，并且该图形项被选中
                else if (selectionManager->isSelected(item)) {
                    QApplication::setOverrideCursor(Qt::SizeAllCursor);
                    cursorSet = true;
                }
            }
        }
        
        // 4. 如果不在控制点上，但在其他选中图形上
        if (!cursorSet) {
            QGraphicsItem* item = drawArea->scene()->itemAt(scenePos, drawArea->transform());
            if (item && selectionManager->isSelected(item)) {
                QApplication::setOverrideCursor(Qt::SizeAllCursor);
                cursorSet = true;
            }
        }
        
        // 5. 如果以上都不是，使用默认光标
        if (!cursorSet) {
            resetCursor(drawArea);
        }
    }
}

void EditState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event)
{
    SelectionManager* selectionManager = getSelectionManager(drawArea);
    if (!selectionManager) {
        return;
    }
    
    // 根据当前状态处理鼠标释放
    if (m_isAreaSelecting) {
        // 完成区域选择
        selectionManager->finishSelection();
        m_isAreaSelecting = false;
    } else if (m_isDragging) {
        // 结束拖动
        m_isDragging = false;
        selectionManager->setDraggingSelection(false);
        
        // 创建移动命令，只有当移动距离足够大时才执行移动
        QPointF scenePos = drawArea->mapToScene(event->pos());
        QPointF delta = scenePos - QPointF(m_dragStartPosition);
        
        // 只有当移动距离足够大时才创建移动命令
        if (delta.manhattanLength() > 3.0) {  // 设置一个小的阈值，避免意外的微小移动
            SelectionCommand* moveCommand = createMoveCommand(drawArea, delta);
            if (moveCommand) {
                CommandManager::getInstance().executeCommand(moveCommand);
            }
        }
    } else if (m_isScaling) {
        // 结束缩放
        m_isScaling = false;
        
        // 获取选中的图形项
        QList<QGraphicsItem*> selectedItems = selectionManager->getSelectedItems();
        if (selectedItems.isEmpty()) {
            resetCursor(drawArea);
            return;
        }
        
        // 创建并执行缩放命令
        TransformCommand* scaleCommand = TransformCommand::createScaleCommand(
            selectedItems, 1.0, selectionManager->selectionCenter());
        
        if (scaleCommand) {
            CommandManager::getInstance().executeCommand(scaleCommand);
            Logger::info("EditState: 执行缩放命令");
        }
    } else if (m_isRotating) {
        // 结束旋转
        m_isRotating = false;
        
        // 这里可以添加旋转命令的执行逻辑
    }
    
    // 全面重置所有操作标志，确保状态干净
    m_isAreaSelecting = false;
    m_isDragging = false;
    m_isScaling = false;
    m_isRotating = false;
    m_activeHandle = GraphicItem::None;
    
    // 重置光标
    resetCursor(drawArea);
    
    // 确保更新视图
    drawArea->viewport()->update();
}

void EditState::keyPressEvent(DrawArea* drawArea, QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete) {
        // 删除选中的图形
        SelectionCommand* command = createDeleteCommand(drawArea);
        if (command) {
            // 直接使用CommandManager的单例模式而不是从DrawArea获取
            CommandManager::getInstance().executeCommand(command);
            Logger::info("EditState: 执行删除命令");
        }
    } else if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier) {
        // 复制选中的图形到内部剪贴板和系统剪贴板
        drawArea->copySelectedItems();
        drawArea->copyToSystemClipboard();
        Logger::info("EditState: 复制选中的图形到剪贴板");
    } else if (event->key() == Qt::Key_V && event->modifiers() == Qt::ControlModifier) {
        // 如果按下了Shift，则粘贴到光标位置
        if (event->modifiers() & Qt::ShiftModifier) {
            // 获取光标位置
            QPoint cursorPos = drawArea->mapFromGlobal(QCursor::pos());
            QPointF scenePos = drawArea->mapToScene(cursorPos);
            
            // 优先从内部剪贴板粘贴
            if (drawArea->canPasteFromClipboard()) {
                drawArea->pasteFromSystemClipboard();
            } else {
                drawArea->pasteItemsAtPosition(scenePos);
            }
            Logger::info("EditState: 粘贴图形到光标位置");
        } else {
            // 常规粘贴（智能定位）
            // 优先从内部剪贴板粘贴
            if (drawArea->canPasteFromClipboard()) {
                drawArea->pasteFromSystemClipboard();
            } else {
                drawArea->pasteItems();
            }
            Logger::info("EditState: 粘贴图形（智能定位）");
        }
    } else if (event->key() == Qt::Key_X && event->modifiers() == Qt::ControlModifier) {
        // 剪切选中的图形
        drawArea->cutSelectedItems();
        Logger::info("EditState: 剪切选中的图形");
    } else if (event->key() == Qt::Key_A && event->modifiers() == Qt::ControlModifier) {
        // 全选
        drawArea->selectAllGraphics();
        Logger::info("EditState: 全选图形");
    }
}

void EditState::paintEvent(DrawArea* drawArea, QPainter* painter)
{
    // 在EditorState中不直接绘制，而是依赖DrawArea的drawForeground来绘制选择控制点
    // 这个方法可以为空，或者用于绘制其他特殊的编辑状态视觉效果
}

// 更新鼠标样式
void EditState::updateCursor(GraphicItem::ControlHandle handle)
{
    // 先恢复默认光标，确保不会积累多个光标
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    
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
            // 默认不做任何事情，因为已经在函数开始时恢复了默认光标
            break;
    }
}

// 处理图形项旋转
void EditState::handleItemRotation(DrawArea* drawArea, const QPointF& pos, GraphicItem* item)
{
    if (!item) {
        return;
    }
    
    // 计算中心点 - 使用场景坐标
    QPointF centerPos = item->mapToScene(item->boundingRect().center());
    
    // 计算当前角度
    double currentAngle = atan2(pos.y() - centerPos.y(), pos.x() - centerPos.x());
    
    // 计算角度差 (弧度)
    double angleDiff = currentAngle - m_initialAngle;
    
    // 转换为度数
    double degrees = angleDiff * 180.0 / M_PI;
    
    // 如果按住Shift键，限制为15度的倍数
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        int steps = qRound(degrees / 15.0);
        degrees = steps * 15.0;
    }
    
    // 应用新的绝对旋转（而不是增量）
    double newRotation = item->rotation() + degrees;
    item->setRotation(newRotation);
    
    // 更新初始角度，以便下次移动时计算增量
    m_initialAngle = currentAngle;
}

// 处理图形项缩放
void EditState::handleItemScaling(DrawArea* drawArea, const QPointF& pos, GraphicItem* item)
{
    if (!item) {
        return;
    }
    
    // 计算中心点 - 使用场景坐标
    QPointF centerPos = item->mapToScene(item->boundingRect().center());
    
    // 根据控制点类型确定缩放方向
    QPointF scaleFactors(1.0, 1.0);
    QPointF currentScale = item->getScale();
    
    // 计算鼠标位置与初始位置的差值
    QPointF delta = pos - m_scaleStartPos;
    
    // 控制缩放速度的敏感度（值越小，缩放越精细）
    const qreal scaleSensitivity = 0.01;
    
    // 计算缩放方向和大小
    switch (m_activeHandle) {
        case GraphicItem::TopLeft:
            // 左上角 - 缩放X和Y
            scaleFactors.setX(1.0 - delta.x() * scaleSensitivity);
            scaleFactors.setY(1.0 - delta.y() * scaleSensitivity);
            break;
        case GraphicItem::TopCenter:
            // 上中 - 只缩放Y
            scaleFactors.setX(1.0);
            scaleFactors.setY(1.0 - delta.y() * scaleSensitivity);
            break;
        case GraphicItem::TopRight:
            // 右上角 - 缩放X和Y
            scaleFactors.setX(1.0 + delta.x() * scaleSensitivity);
            scaleFactors.setY(1.0 - delta.y() * scaleSensitivity);
            break;
        case GraphicItem::MiddleLeft:
            // 左中 - 只缩放X
            scaleFactors.setX(1.0 - delta.x() * scaleSensitivity);
            scaleFactors.setY(1.0);
            break;
        case GraphicItem::MiddleRight:
            // 右中 - 只缩放X
            scaleFactors.setX(1.0 + delta.x() * scaleSensitivity);
            scaleFactors.setY(1.0);
            break;
        case GraphicItem::BottomLeft:
            // 左下角 - 缩放X和Y
            scaleFactors.setX(1.0 - delta.x() * scaleSensitivity);
            scaleFactors.setY(1.0 + delta.y() * scaleSensitivity);
            break;
        case GraphicItem::BottomCenter:
            // 下中 - 只缩放Y
            scaleFactors.setX(1.0);
            scaleFactors.setY(1.0 + delta.y() * scaleSensitivity);
            break;
        case GraphicItem::BottomRight:
            // 右下角 - 缩放X和Y
            scaleFactors.setX(1.0 + delta.x() * scaleSensitivity);
            scaleFactors.setY(1.0 + delta.y() * scaleSensitivity);
            break;
        default:
            break;
    }
    
    // 计算新的缩放值
    QPointF newScale(currentScale.x() * scaleFactors.x(), currentScale.y() * scaleFactors.y());
    
    // 确保缩放不会太小
    const qreal minScale = 0.1;
    newScale.setX(qMax(minScale, newScale.x()));
    newScale.setY(qMax(minScale, newScale.y()));
    
    // 应用新的缩放
    item->setScale(newScale);
    
    // 更新起始位置为当前位置，以便下一次移动计算增量
    m_scaleStartPos = pos;
}

// 创建移动命令
SelectionCommand* EditState::createMoveCommand(DrawArea* drawArea, const QPointF& offset)
{
    // 如果偏移量太小，不创建命令
    if (offset.manhattanLength() < 3.0) {
        return nullptr;
    }
    
    QList<QGraphicsItem*> selectedItems;
    
    if (getSelectionManager(drawArea)->isDraggingSelection()) {
        selectedItems = getSelectionManager(drawArea)->getSelectedItems();
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

// 创建样式变更命令
StyleChangeCommand* EditState::createStyleChangeCommand(DrawArea* drawArea, 
                                                     StyleChangeCommand::StylePropertyType propertyType)
{
    QList<QGraphicsItem*> selectedItems = drawArea->scene()->selectedItems();
    
    if (selectedItems.isEmpty()) {
        Logger::debug("EditState: 没有选中的图形项，无法创建样式变更命令");
        return nullptr;
    }
    
    // 直接使用QList而不是转换为std::vector
    StyleChangeCommand* styleCmd = new StyleChangeCommand(drawArea, selectedItems, propertyType);
    
    Logger::debug(QString("EditState: 创建样式变更命令 - 选中项数量: %1, 属性类型: %2")
             .arg(selectedItems.size())
             .arg(static_cast<int>(propertyType)));
    
    return styleCmd;
}

// 应用画笔颜色变更
void EditState::applyPenColorChange(const QColor &color)
{
    if (!m_drawArea) {
        Logger::warning("EditState::applyPenColorChange: DrawArea为空");
        return;
    }
    
    QList<QGraphicsItem *> selectedItems = m_drawArea->scene()->selectedItems();
    if (selectedItems.isEmpty()) {
        Logger::warning("EditState::applyPenColorChange: 没有选中的图形项");
        return;
    }
    
    Logger::debug(QString("EditState::applyPenColorChange: 选中了 %1 个图形项").arg(selectedItems.size()));
    
    // 创建风格变更命令
    auto command = new StyleChangeCommand(m_drawArea,
                                         selectedItems,
                                         StyleChangeCommand::PenColor);
    command->setNewPenColor(color);
    
    CommandManager::getInstance().executeCommand(command);
    Logger::info(QString("EditState::applyPenColorChange: 成功将颜色更改为 %1").arg(color.name()));
}

// 应用画笔宽度变更
void EditState::applyPenWidthChange(qreal width)
{
    if (!m_drawArea) {
        Logger::warning("EditState::applyPenWidthChange: DrawArea为空");
        return;
    }
    
    QList<QGraphicsItem *> selectedItems = m_drawArea->scene()->selectedItems();
    if (selectedItems.isEmpty()) {
        Logger::warning("EditState::applyPenWidthChange: 没有选中的图形项");
        return;
    }
    
    Logger::debug(QString("EditState::applyPenWidthChange: 选中了 %1 个图形项").arg(selectedItems.size()));
    
    // 创建风格变更命令
    auto command = new StyleChangeCommand(m_drawArea,
                                         selectedItems,
                                         StyleChangeCommand::PenWidth);
    command->setNewPenWidth(width);
    
    CommandManager::getInstance().executeCommand(command);
    Logger::info(QString("EditState::applyPenWidthChange: 成功将线宽更改为 %1").arg(width));
}

// 应用画刷颜色变更
void EditState::applyBrushColorChange(const QColor &color)
{
    if (!m_drawArea) {
        Logger::warning("EditState::applyBrushColorChange: DrawArea为空");
        return;
    }
    
    auto drawArea = m_drawArea;
    QList<QGraphicsItem *> selectedItems = drawArea->scene()->selectedItems();
    if (selectedItems.isEmpty()) {
        Logger::warning("EditState::applyBrushColorChange: 没有选中的图形项");
        return;
    }
    
    Logger::debug(QString("EditState::applyBrushColorChange: 选中了 %1 个图形项").arg(selectedItems.size()));
    
    StyleChangeCommand* command = new StyleChangeCommand(drawArea,
                                       selectedItems,
                                       StyleChangeCommand::BrushColor);
    command->setNewBrushColor(color);
    
    CommandManager::getInstance().executeCommand(command);
    Logger::info(QString("EditState::applyBrushColorChange: 成功将填充颜色更改为 %1").arg(color.name()));
}

// 添加新的辅助方法来获取SelectionManager
SelectionManager* EditState::getSelectionManager(DrawArea* drawArea) const
{
    if (!drawArea) {
        Logger::warning("EditState::getSelectionManager: 传入的DrawArea为空");
        return nullptr;
    }
    return drawArea->getSelectionManager();
} 