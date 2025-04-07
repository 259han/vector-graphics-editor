#include "draw_area.h"
#include "../state/draw_state.h"
#include "../state/edit_state.h"
#include "../state/fill_state.h"
#include "../core/graphic_item.h"
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QPainter>
#include <QDebug>
#include "../core/graphics_item_factory.h"
#include "../state/editor_state.h"
#include <QApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QDateTime>
#include <QInputDialog>
#include <QFileInfo>
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include "ui/image_resizer.h"
#include "core/image_manager.h"
#include "core/selection_manager.h"
#include "../utils/logger.h"

DrawArea::DrawArea(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_currentState(nullptr)
    , m_imageManager(new ImageManager(this))
    , m_graphicFactory(std::make_unique<DefaultGraphicsItemFactory>())
    , m_selectionManager(std::make_unique<SelectionManager>(m_scene))
{
    // 创建场景
    setScene(m_scene);
    
    // 设置视图属性
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // 设置场景大小为更合理的尺寸（从-5000,-5000,10000,10000改为-1000,-1000,2000,2000）
    m_scene->setSceneRect(-1000, -1000, 2000, 2000);
    
    // 设置初始编辑状态
    setEditState();
    
    // 允许背景绘制
    setBackgroundBrush(Qt::white);
    
    // 允许拖放
    setAcceptDrops(true);
    
    // 设置图像管理器
    m_imageManager->setDrawArea(this);
    
    // 连接选择管理器的信号
    connect(m_selectionManager.get(), &SelectionManager::selectionChanged, [this]() {
        viewport()->update();
    });
}

DrawArea::~DrawArea()
{
    // 清理图像调整器
    qDeleteAll(m_imageResizers);
    m_imageResizers.clear();
    
    // 图像管理器会由Qt的父子关系自动清理
    // m_scene会由Qt的父子关系自动清理
}

void DrawArea::setDrawState(Graphic::GraphicType graphicType)
{
    // 先保存当前状态，以便于在切换状态时通知
    auto oldState = m_currentState.get();
    
    // 在切换前通知当前状态即将退出
    if (oldState) {
        oldState->onExitState(this);
    }
    
    // 清理之前的状态
    m_currentState.reset();
    // 设置新的状态
    m_currentState = std::make_unique<DrawState>(graphicType);
    qDebug() << "DrawArea: 切换到绘制状态，图形类型:" << static_cast<int>(graphicType);
    setCursor(Qt::CrossCursor);
    
    // 通知新状态已经进入
    m_currentState->onEnterState(this);
}

void DrawArea::setEditState()
{
    // 先保存当前状态，以便于在切换状态时通知
    Logger::debug("DrawArea::setEditState: 开始切换到编辑状态");
    auto oldState = m_currentState.get();
    
    // 在切换前通知当前状态即将退出
    if (oldState) {
        Logger::debug("DrawArea::setEditState: 通知当前状态即将退出");
        oldState->onExitState(this);
        Logger::debug("DrawArea::setEditState: 当前状态已退出");
    }
    
    // 确保UI能响应
    QApplication::processEvents();
    
    // 切换到编辑状态
    Logger::debug("DrawArea::setEditState: 创建新的编辑状态");
    m_currentState = std::make_unique<EditState>();
    Logger::debug("DrawArea::setEditState: 编辑状态已创建");
    
    // 确保UI能响应
    QApplication::processEvents();
    
    // 通知新状态已经进入
    Logger::debug("DrawArea::setEditState: 通知编辑状态已进入");
    m_currentState->onEnterState(this);
    Logger::debug("DrawArea::setEditState: 编辑状态初始化完成");
    
    // 确保场景中所有图形项都能被选择
    if (m_scene) {
        Logger::debug("DrawArea::setEditState: 设置场景中的图形项为可选择状态");
        int count = 0;
        for (QGraphicsItem* item : m_scene->items()) {
            if (!item->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
                item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                count++;
            }
        }
        Logger::debug(QString("DrawArea::setEditState: 已设置 %1 个图形项为可选择状态").arg(count));
    }
    
    // 确保UI更新
    viewport()->update();
    if (m_scene) {
        m_scene->update();
    }
    QApplication::processEvents();
    
    Logger::debug("DrawArea::setEditState: 切换到编辑状态完成");
}

void DrawArea::setFillState()
{
    // 先保存当前状态，以便于在切换状态时通知
    auto oldState = m_currentState.get();
    
    // 在切换前通知当前状态即将退出
    if (oldState) {
        oldState->onExitState(this);
    }
    
    // 清理选择状态
    scene()->clearSelection();
    
    // 创建新的填充状态，并传入当前的填充颜色
    m_currentState = std::make_unique<FillState>(m_fillColor);
    
    // 设置填充工具的光标
    setCursor(QCursor(QPixmap(":/icons/bucket_cursor.png"), 0, 15)); // 如果有自定义光标图片
    if (cursor().shape() == Qt::ArrowCursor) {
        // 如果没有自定义光标，则使用指向手
        setCursor(Qt::PointingHandCursor);
    }
    
    qDebug() << "切换到填充状态，填充颜色:" << m_fillColor;
    
    // 通知新状态已经进入
    m_currentState->onEnterState(this);
}

void DrawArea::setFillState(const QColor& color)
{
    // 先设置填充颜色
    m_fillColor = color;
    
    // 然后切换到填充状态
    setFillState();
}

void DrawArea::setFillColor(const QColor& color)
{
    m_fillColor = color;
    if (auto* fillState = dynamic_cast<FillState*>(m_currentState.get())) {
        fillState->setFillColor(color);
    }
    qDebug() << "设置填充颜色:" << color;
}

DefaultGraphicsItemFactory* DrawArea::getGraphicFactory()
{
    return m_graphicFactory.get();
}

void DrawArea::enableGrid(bool enable)
{
    m_gridEnabled = enable;
    viewport()->update();
}

void DrawArea::setGridSize(int size)
{
    m_gridSize = size;
    viewport()->update();
}

void DrawArea::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawBackground(painter, rect);
    
    // 只有在启用网格时才绘制
    if (m_gridEnabled) {
        // 计算网格线
        qreal left = int(rect.left()) - (int(rect.left()) % m_gridSize);
        qreal top = int(rect.top()) - (int(rect.top()) % m_gridSize);
        
        // 设置网格线画笔
        QPen gridPen(QColor(220, 220, 220), 0.5);
        painter->setPen(gridPen);
        
        // 绘制垂直网格线
        QVarLengthArray<QLineF, 100> lines;
        for (qreal x = left; x < rect.right(); x += m_gridSize) {
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        }
        
        // 绘制水平网格线
        for (qreal y = top; y < rect.bottom(); y += m_gridSize) {
            lines.append(QLineF(rect.left(), y, rect.right(), y));
        }
        
        painter->drawLines(lines.data(), lines.size());
    }
}

void DrawArea::wheelEvent(QWheelEvent *event)
{
    // 滚轮缩放
    if (event->modifiers() & Qt::ControlModifier) {
        // 放大/缩小灵敏度增加
        double scaleFactor = 1.15;
        if (event->angleDelta().y() < 0) {
            scaleFactor = 1.0 / scaleFactor;
        }
        scale(scaleFactor, scaleFactor);
    } else {
        // 正常滚动
        QGraphicsView::wheelEvent(event);
    }
}

void DrawArea::mousePressEvent(QMouseEvent *event)
{
    // 空格键按下时进入平移模式
    if (event->button() == Qt::LeftButton && m_spaceKeyPressed) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    
    // 这里不重置事件接受状态，确保子类处理能正确传递
    event->setAccepted(false);
    
    // 委托给当前状态处理
    if (m_currentState) {
        QPointF scenePos = mapToScene(event->pos());
        qDebug() << "DrawArea: 鼠标按下事件，位置:" << scenePos << "，按钮:" 
                 << (event->button() == Qt::LeftButton ? "左键" : 
                    (event->button() == Qt::RightButton ? "右键" : "其他"));
        
        if (event->button() == Qt::LeftButton) {
            m_currentState->handleLeftMousePress(this, scenePos);
        } else if (event->button() == Qt::RightButton) {
            m_currentState->handleRightMousePress(this, scenePos);
        } else {
            // 调用旧的mousePressEvent接口保持兼容性
            m_currentState->mousePressEvent(this, event);
        }
        
        // 确保视口更新 - 这一步很重要，强制重绘
        viewport()->update();
        scene()->update();
        
        // 如果事件已被接受，则不继续传递
        if (event->isAccepted()) {
            return;
        }
        
        // 如果使用了绘制状态，则不要继续传递事件到QGraphicsView
        // 即使事件未被明确接受
        if (dynamic_cast<DrawState*>(m_currentState.get())) {
            event->accept();
            return;
        }
    }
    
    // 最后才调用基类方法
    QGraphicsView::mousePressEvent(event);
}

void DrawArea::mouseMoveEvent(QMouseEvent *event)
{
    // 平移模式下处理画布平移
    if (m_isPanning) {
        // 将QPointF转换为QPoint
        QPointF delta = mapToScene(event->pos()) - mapToScene(m_lastPanPoint.toPoint());
        m_lastPanPoint = event->pos();
        
        // 平移视图
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        
        event->accept();
        return;
    }
    
    // 这里不重置事件接受状态，确保子类处理能正确传递
    event->setAccepted(false);
    
    // 委托给当前状态处理
    if (m_currentState) {
        m_currentState->mouseMoveEvent(this, event);
        
        // 确保视口更新
        viewport()->update();
        scene()->update();
        
        // 如果事件已被接受，则不继续传递
        if (event->isAccepted()) {
            return;
        }
        
        // 如果使用了绘制状态，则不要继续传递事件到QGraphicsView
        // 即使事件未被明确接受
        if (dynamic_cast<DrawState*>(m_currentState.get())) {
            event->accept();
            return;
        }
    }
    
    // 最后才调用基类方法
    QGraphicsView::mouseMoveEvent(event);
}

void DrawArea::mouseReleaseEvent(QMouseEvent *event)
{
    // 平移模式结束
    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    
    // 这里不重置事件接受状态，确保子类处理能正确传递
    event->setAccepted(false);
    
    // 委托给当前状态处理
    if (m_currentState) {
        // 调用当前状态的鼠标释放事件处理
        m_currentState->mouseReleaseEvent(this, event);
        
        // 确保视口更新
        viewport()->update();
        scene()->update();
        
        // 如果事件已被接受，则不继续传递
        if (event->isAccepted()) {
            return;
        }
        
        // 如果使用了绘制状态，则不要继续传递事件到QGraphicsView
        // 即使事件未被明确接受
        if (dynamic_cast<DrawState*>(m_currentState.get())) {
            event->accept();
            return;
        }
    }
    
    // 最后才调用基类方法
    QGraphicsView::mouseReleaseEvent(event);
}

void DrawArea::keyPressEvent(QKeyEvent *event)
{
    // 处理空格键（进入准备平移状态）
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceKeyPressed = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    
    // 处理Ctrl键
    if (event->key() == Qt::Key_Control) {
        m_ctrlKeyPressed = true;
        if (m_selectionManager) {
            m_selectionManager->setSelectionMode(SelectionManager::MultiSelection);
        }
    }
    
    // 委托给当前状态处理
    if (m_currentState) {
        m_currentState->keyPressEvent(this, event);
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void DrawArea::keyReleaseEvent(QKeyEvent *event)
{
    // 处理空格键释放
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceKeyPressed = false;
        setCursor(m_isPanning ? Qt::ClosedHandCursor : Qt::ArrowCursor);
        event->accept();
        return;
    }
    
    // 处理Ctrl键释放
    if (event->key() == Qt::Key_Control) {
        m_ctrlKeyPressed = false;
        if (m_selectionManager) {
            m_selectionManager->setSelectionMode(SelectionManager::SingleSelection);
        }
    }
    
    QGraphicsView::keyReleaseEvent(event);
    
    // 调用当前状态的键盘事件处理
    if (m_currentState && m_currentState->getStateType() == EditorState::EditState) {
        m_currentState->keyReleaseEvent(this, event);
    }
}

void DrawArea::clearGraphics()
{
    Logger::info("DrawArea::clearGraphics: 开始清空场景");
    
    // 首先清除所有选择状态，以避免删除时引用已删除的对象
    if (m_selectionManager) {
        Logger::debug("DrawArea::clearGraphics: 清除选择状态");
        m_selectionManager->clearSelection();
    }
    
    // 记录场景中项目的数量
    int itemCount = (m_scene != nullptr) ? m_scene->items().count() : 0;
    Logger::debug(QString("DrawArea::clearGraphics: 准备清除 %1 个项目").arg(itemCount));
    
    // 手动清除场景中的每个项目，而不是直接调用clear()
    if (m_scene) {
        // 先获取所有项目的副本
        QList<QGraphicsItem*> itemsToRemove = m_scene->items();
        
        // 逐个删除项目
        for (QGraphicsItem* item : itemsToRemove) {
            if (item) {
                Logger::debug(QString("DrawArea::clearGraphics: 删除项目 %1").arg(reinterpret_cast<quintptr>(item)));
                m_scene->removeItem(item);
                delete item;
                
                // 不在循环中处理事件，避免递归事件处理
                // 如果真的需要处理事件，可以每删除许多项后处理一次
                // 如果场景项特别多，可以考虑分批删除
            }
        }
        
        // 最后再调用clear确保清空
        m_scene->clear();
        
        Logger::debug("DrawArea::clearGraphics: 所有项目已清除");
    } else {
        Logger::warning("DrawArea::clearGraphics: 场景为空，无需清除");
    }
    
    // 更新视图
    if (m_scene) {
        m_scene->update();
    }
    viewport()->update();
    
    // 在完成所有清理后一次性处理事件，而不是在循环中多次处理
    QApplication::processEvents();
    
    Logger::info("DrawArea::clearGraphics: 场景清空完成");
}

void DrawArea::setImage(const QImage &image)
{
    if (image.isNull()) {
        qWarning() << "DrawArea::setImage: Attempted to set null image";
        return;
    }
    
    // 委托给ImageManager处理
    m_imageManager->addImageToScene(image);
}

void DrawArea::saveImage()
{
    m_imageManager->saveImage(QString());
}

void DrawArea::importImage()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("导入图像"),
        QString(),
        tr("图像文件 (*.png *.jpg *.jpeg *.bmp *.gif *.tif *.tiff);;所有文件 (*)")
    );
    
    if (!fileName.isEmpty()) {
        m_imageManager->importImage(fileName);
    }
}

void DrawArea::moveSelectedGraphics(const QPointF& offset)
{
    if (m_selectionManager) {
        m_selectionManager->moveSelection(offset);
    }
}

void DrawArea::rotateSelectedGraphics(double angle)
{
    if (!m_selectionManager || m_selectionManager->getSelectedItems().isEmpty()) {
        return;
    }
    
    // 获取选择中心点作为旋转中心
    QPointF center = m_selectionManager->selectionCenter();
    
    // 对每个选中的图形项应用旋转
    for (QGraphicsItem* item : m_selectionManager->getSelectedItems()) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (graphicItem) {
            // 计算相对于中心点的旋转
            QPointF itemPos = item->pos();
            QPointF relativePos = itemPos - center;
            
            // 应用旋转变换
            QTransform transform;
            transform.translate(center.x(), center.y())
                    .rotate(angle)
                    .translate(-center.x(), -center.y());
            
            QPointF newPos = transform.map(itemPos);
            item->setPos(newPos);
            
            // 旋转图形自身
            graphicItem->rotateBy(angle);
        }
    }
    
    // 更新视图
    viewport()->update();
}

void DrawArea::scaleSelectedGraphics(double factor)
{
    if (!m_selectionManager || m_selectionManager->getSelectedItems().isEmpty()) {
        return;
    }
    
    // 获取选择中心点作为缩放中心
    QPointF center = m_selectionManager->selectionCenter();
    
    // 对每个选中的图形项应用缩放
    for (QGraphicsItem* item : m_selectionManager->getSelectedItems()) {
        GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (graphicItem) {
            // 计算相对于中心点的缩放
            QPointF itemPos = item->pos();
            QPointF relativePos = itemPos - center;
            QPointF newRelativePos = relativePos * factor;
            QPointF newPos = center + newRelativePos;
            
            // 设置新位置
            item->setPos(newPos);
            
            // 缩放图形自身
            graphicItem->scaleBy(factor);
        }
    }
    
    // 更新视图
    viewport()->update();
}

void DrawArea::deleteSelectedGraphics()
{
    if (!m_selectionManager) {
        return;
    }
    
    // 获取选中的图形项
    QList<QGraphicsItem*> selectedItems = m_selectionManager->getSelectedItems();
    
    // 从场景中移除图形项
    for (QGraphicsItem* item : selectedItems) {
        m_scene->removeItem(item);
        delete item;
    }
    
    // 清除选择
    m_selectionManager->clearSelection();
    
    // 更新视图
    viewport()->update();
}

void DrawArea::selectAllGraphics()
{
    if (!m_selectionManager || !m_scene) {
        return;
    }
    
    // 清除当前选择
    m_selectionManager->clearSelection();
    
    // 获取所有图形项
    QList<QGraphicsItem*> allItems = m_scene->items();
    
    // 选择所有通过过滤器的图形项
    for (QGraphicsItem* item : allItems) {
        if (item && item->type() != QGraphicsRectItem::Type) {
            m_selectionManager->addToSelection(item);
        }
    }
    
    // 更新视图
    viewport()->update();
}

void DrawArea::bringToFront(QGraphicsItem* item)
{
    if (!item) return;
    
    // 找到最高的Z值并设置当前项更高一级
    qreal maxZ = -1;
    for (QGraphicsItem* otherItem : m_scene->items()) {
        if (otherItem != item && otherItem->zValue() > maxZ) {
            maxZ = otherItem->zValue();
        }
    }
    
    item->setZValue(maxZ + 1);
}

void DrawArea::sendToBack(QGraphicsItem* item)
{
    if (!item) return;
    
    // 找到最低的Z值并设置当前项更低一级
    qreal minZ = 1;
    for (QGraphicsItem* otherItem : m_scene->items()) {
        if (otherItem != item && otherItem->zValue() < minZ) {
            minZ = otherItem->zValue();
        }
    }
    
    item->setZValue(minZ - 1);
}

void DrawArea::bringForward(QGraphicsItem* item)
{
    if (!item) return;
    
    // 找到比当前项高一级的Z值
    qreal currentZ = item->zValue();
    qreal nextZ = currentZ;
    
    for (QGraphicsItem* otherItem : m_scene->items()) {
        if (otherItem != item && otherItem->zValue() > currentZ) {
            if (otherItem->zValue() < nextZ || nextZ == currentZ) {
                nextZ = otherItem->zValue();
            }
        }
    }
    
    // 如果找到了更高一级的Z值，交换两者
    if (nextZ > currentZ) {
        for (QGraphicsItem* otherItem : m_scene->items()) {
            if (otherItem->zValue() == nextZ) {
                otherItem->setZValue(currentZ);
                break;
            }
        }
        item->setZValue(nextZ);
    } else {
        // 如果没有更高级别，直接提升一级
        item->setZValue(currentZ + 1);
    }
}

void DrawArea::sendBackward(QGraphicsItem* item)
{
    if (!item) return;
    
    // 找到比当前项低一级的Z值
    qreal currentZ = item->zValue();
    qreal prevZ = currentZ;
    
    for (QGraphicsItem* otherItem : m_scene->items()) {
        if (otherItem != item && otherItem->zValue() < currentZ) {
            if (otherItem->zValue() > prevZ || prevZ == currentZ) {
                prevZ = otherItem->zValue();
            }
        }
    }
    
    // 如果找到了更低一级的Z值，交换两者
    if (prevZ < currentZ) {
        for (QGraphicsItem* otherItem : m_scene->items()) {
            if (otherItem->zValue() == prevZ) {
                otherItem->setZValue(currentZ);
                break;
            }
        }
        item->setZValue(prevZ);
    } else {
        // 如果没有更低级别，直接降低一级
        item->setZValue(currentZ - 1);
    }
}

// 获取选中的图形项
QList<QGraphicsItem*> DrawArea::getSelectedItems() const
{
    if (m_selectionManager) {
        return m_selectionManager->getSelectedItems();
    }
    return QList<QGraphicsItem*>();
}

// 复制选中的图形项
void DrawArea::copySelectedItems() {
    auto selectedItems = getSelectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    // 清空复制缓冲区
    m_copyBuffer.clear();
    
    // 将选中项添加到复制缓冲区
    for (auto item : selectedItems) {
        if (auto* graphicItem = dynamic_cast<GraphicItem*>(item)) {
            // 在这里可以深拷贝图形项，但现在先简单保存引用
            m_copyBuffer.append(item);
        }
    }
}

// 粘贴复制的图形项
void DrawArea::pasteItems() {
    if (m_copyBuffer.isEmpty() || !m_scene) {
        return;
    }
    
    // 记录当前选中项
    auto currentSelected = m_scene->selectedItems();
    
    // 取消所有选择
    for (auto item : currentSelected) {
        item->setSelected(false);
    }
    
    // 粘贴复制的图形项
    QPointF offset(20, 20); // 粘贴时的偏移量
    
    for (auto item : m_copyBuffer) {
        if (auto* graphicItem = dynamic_cast<GraphicItem*>(item)) {
            // 根据图形类型创建新图形
            auto type = graphicItem->getGraphicType();
            QGraphicsItem* newItem = m_graphicFactory->createItem(type, graphicItem->pos() + offset);
            
            if (newItem) {
                // 复制属性（如果需要）
                if (auto* newGraphicItem = dynamic_cast<GraphicItem*>(newItem)) {
                    newGraphicItem->setPen(graphicItem->getPen());
                    newGraphicItem->setBrush(graphicItem->getBrush());
                    // 可以复制更多属性
                }
                
                m_scene->addItem(newItem);
                newItem->setSelected(true);
            }
        }
    }
}

// 剪切选中的图形项
void DrawArea::cutSelectedItems() {
    copySelectedItems();
    deleteSelectedGraphics();
}

void DrawArea::addImageResizer(ImageResizer* resizer)
{
    if (resizer) {
        m_imageResizers.append(resizer);
    }
}

void DrawArea::setLineColor(const QColor& color)
{
    m_lineColor = color;
    
    // 如果当前是绘制状态，则设置绘制状态的线条颜色
    DrawState* drawState = dynamic_cast<DrawState*>(m_currentState.get());
    if (drawState) {
        drawState->setLineColor(color);
    }
    
    qDebug() << "DrawArea: 设置线条颜色为" << color;
}

void DrawArea::setLineWidth(int width)
{
    m_lineWidth = width;
    
    // 如果当前是绘制状态，则设置绘制状态的线条宽度
    DrawState* drawState = dynamic_cast<DrawState*>(m_currentState.get());
    if (drawState) {
        drawState->setLineWidth(width);
    }
    
    qDebug() << "DrawArea: 设置线条宽度为" << width;
}

void DrawArea::setSelectionMode(SelectionManager::SelectionMode mode)
{
    if (m_selectionManager) {
        m_selectionManager->setSelectionMode(mode);
    }
}

void DrawArea::setSelectionFilter(const SelectionManager::SelectionFilter& filter)
{
    if (m_selectionManager) {
        m_selectionManager->setSelectionFilter(filter);
    }
}

void DrawArea::clearSelectionFilter()
{
    if (m_selectionManager) {
        m_selectionManager->clearSelectionFilter();
    }
}

// 实现drawForeground用于绘制选择控制点
void DrawArea::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawForeground(painter, rect);
    
    // 使用SelectionManager绘制选择控制点
    if (m_selectionManager && !m_selectionManager->getSelectedItems().isEmpty()) {
        // 保存当前的转换矩阵
        painter->save();
        
        // 应用视图到场景的转换矩阵的逆变换，确保在场景坐标系中绘制
        painter->setTransform(viewportTransform().inverted(), true);
        
        // 现在可以安全地在场景坐标系统中绘制，传递this作为QGraphicsView*
        m_selectionManager->paint(painter, this);
        
        // 恢复原始转换矩阵
        painter->restore();
    }
}