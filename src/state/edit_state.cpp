#include "state/edit_state.h"
#include "ui/draw_area.h"
#include "core/graphic_item.h"
#include "core/selection_manager.h"
// 裁剪功能已移至future/clip目录
// #include "core/graphics_clipper.h"
#include "command/selection_command.h"
#include "command/command_manager.h"
#include "command/style_change_command.h"
#include "../utils/logger.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QPen>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QTransform>
#include <QGraphicsPathItem>
#include <QtMath>

EditState::EditState()
    : m_isAreaSelecting(false),
      m_selectionStart(),
      m_isDragging(false),
      m_dragStartPosition(),
      m_activeHandle(GraphicItem::None),
      m_isRotating(false),
      m_isScaling(false),
      m_initialAngle(0.0),
      m_transformOrigin()
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
    qDebug() << "Edit State: 左键点击，位置:" << scenePos;
    
    // 获取选择管理器
    SelectionManager* selectionManager = getSelectionManager(drawArea);
    if (!selectionManager) {
        return;
    }
    
    // 检查点击位置是否有图形项或选择区域
    QGraphicsScene* scene = drawArea->scene();
    QGraphicsItem* item = scene->itemAt(scenePos, drawArea->transform());
    
    // 检查点击是否在选择区域上
    if (selectionManager->isSelectionValid() && 
        (selectionManager->contains(scenePos) || selectionManager->handleAtPoint(scenePos) != GraphicItem::None)) {
        
        // 获取选择区域上的控制点
        m_activeHandle = selectionManager->handleAtPoint(scenePos);
        
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
            selectionManager->setDraggingSelection(true);
            qDebug() << "Edit State: Preparing to drag selection area";
            return;
        }
    }
    
    // 如果点击了某个普通图形项
    if (item) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (graphicItem) {
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
                
                // 更新光标
                updateCursor(m_activeHandle);
                return;
            }
            
            // 正常选择图形项
            bool ctrlPressed = QApplication::keyboardModifiers() & Qt::ControlModifier;
            if (ctrlPressed) {
                // Ctrl+点击，切换图形选择状态
                selectionManager->toggleSelection(item);
                
                // 使用多选模式
                selectionManager->setSelectionMode(SelectionManager::MultiSelection);
            } else {
                // 单击，如果项没被选中，先清除当前选择再选中这一项
                if (!selectionManager->isSelected(item)) {
                    selectionManager->clearSelection();
                    selectionManager->addToSelection(item);
                }
                
                // 使用单选模式
                selectionManager->setSelectionMode(SelectionManager::SingleSelection);
            }
            
            // 记录起始拖动位置
            m_dragStartPosition = QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
            m_isDragging = true;
            return;
        }
    }
    
    // 点击在空白处，开始区域选择
    m_isAreaSelecting = true;
    m_selectionStart = QPoint(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
    selectionManager->startSelection(scenePos);
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
    } else if (m_isDragging) {
        // 移动选中的图形
        QPoint newPos(static_cast<int>(scenePos.x()), static_cast<int>(scenePos.y()));
        QPoint delta = newPos - m_dragStartPosition;
        
        // 只有当移动距离超过阈值时才真正移动对象
        // 这可以防止在简单点击选择对象时意外的微小移动
        if (QPointF(delta).manhattanLength() > 3.0) {
            QPointF deltaF(delta.x(), delta.y());
            
            // 移动选中的图形
            drawArea->moveSelectedGraphics(deltaF);
            
            // 更新拖动起始位置
            m_dragStartPosition = newPos;
        }
    } else if (m_isRotating) {
        // 处理旋转
        if (!drawArea->getSelectedItems().isEmpty()) {
            QGraphicsItem* item = drawArea->getSelectedItems().first();
            GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
            if (graphicItem) {
                handleItemRotation(drawArea, scenePos, graphicItem);
            }
        }
    } else if (m_isScaling) {
        // 处理缩放
        if (m_activeHandle != GraphicItem::None) {
            selectionManager->scaleSelection(m_activeHandle, scenePos);
        }
    } else {
        // 未按下鼠标时，更新光标样式
        // 先检查是否在选择区域的控制点上
        GraphicItem::ControlHandle handle = selectionManager->handleAtPoint(scenePos);
        if (handle != GraphicItem::None) {
            updateCursor(handle);
            return;
        }
        
        // 检查是否在图形项的控制点上
        QGraphicsItem* item = drawArea->scene()->itemAt(scenePos, drawArea->transform());
        if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
            handle = graphicItem->handleAtPoint(scenePos);
            if (handle != GraphicItem::None) {
                updateCursor(handle);
                return;
            }
        }
        
        // 不在控制点上时使用默认光标
        resetCursor(drawArea);
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
    } else if (m_isRotating) {
        // 结束旋转
        m_isRotating = false;
        
        // 创建旋转命令
        // TODO: 实现旋转命令
    } else if (m_isScaling) {
        // 结束缩放
        m_isScaling = false;
        
        // 创建缩放命令
        // TODO: 实现缩放命令
    }
    
    // 重置操作标志和光标
    m_activeHandle = GraphicItem::None;
    resetCursor(drawArea);
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