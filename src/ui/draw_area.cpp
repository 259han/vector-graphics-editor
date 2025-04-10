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
#include <QDataStream>
#include <QMenu>
#include <QContextMenuEvent>
#include "../command/paste_command.h"
#include "../command/command_manager.h"

// Define the static MIME type constant
const QString DrawArea::MIME_GRAPHICITEMS = "application/x-claudegraph-items";

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
    connect(m_selectionManager.get(), &SelectionManager::selectionChanged, this, [this]() {
        viewport()->update();
        emit selectionChanged();
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

// 选择所有图形项
void DrawArea::selectAllGraphics() {
    if (!m_scene) {
        return;
    }
    
    // 获取场景中的所有可选择图形项
    QList<QGraphicsItem*> allItems = m_scene->items();
    
    // 取消当前选择
    for (auto item : m_scene->selectedItems()) {
        item->setSelected(false);
    }
    
    // 选择所有图形项
    for (auto item : allItems) {
        if (dynamic_cast<GraphicItem*>(item)) {
            item->setSelected(true);
        }
    }
    
    // 如果需要，同步选择管理器的状态
    if (m_selectionManager) {
        m_selectionManager->syncSelectionFromScene();
    }
    
    Logger::info(QString("已选择所有图形项: %1个").arg(m_scene->selectedItems().size()));
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

// 复制选中的图形项到内部剪贴板
void DrawArea::copySelectedItems() {
    auto selectedItems = getSelectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    // 清空内部剪贴板数据
    m_clipboardData.clear();
    
    // 保存选中项到内部剪贴板
    for (auto item : selectedItems) {
        if (auto* graphicItem = dynamic_cast<GraphicItem*>(item)) {
            saveGraphicItemToClipboard(graphicItem);
        }
    }
    
    Logger::info(QString("已复制 %1 个图形项到内部剪贴板").arg(m_clipboardData.size()));
}

// 粘贴复制的图形项（在默认位置）
void DrawArea::pasteItems() {
    // 使用智能定位
    pasteItemsAtPosition(calculateSmartPastePosition());
}

// 在指定位置粘贴图形项
void DrawArea::pasteItemsAtPosition(const QPointF& pos) {
    // 防止重复执行粘贴操作
    static bool isPasting = false;
    if (isPasting) {
        Logger::warning("DrawArea::pasteItemsAtPosition: 已有粘贴操作正在执行，防止重复粘贴");
        return;
    }
    
    // 设置标志，防止重入
    isPasting = true;
    
    // 进行标准检查
    if (m_clipboardData.isEmpty() || !m_scene) {
        Logger::warning("DrawArea::pasteItemsAtPosition: 剪贴板为空或场景无效，无法粘贴");
        isPasting = false;
        return;
    }
    
    Logger::debug(QString("DrawArea::pasteItemsAtPosition: 开始粘贴 %1 个项目到位置 (%2, %3)")
                 .arg(m_clipboardData.size())
                 .arg(pos.x())
                 .arg(pos.y()));
    
    try {
        // 开始命令分组，确保粘贴操作是一个独立的命令
        CommandManager::getInstance().beginCommandGroup();
        
        // 记录当前选中项
        auto currentSelected = m_scene->selectedItems();
        
        // 取消所有选择
        for (auto item : currentSelected) {
            item->setSelected(false);
        }
        
        // 计算图形项的边界矩形，用于居中
        QRectF boundingRect;
        bool first = true;
        
        for (const auto& clipData : m_clipboardData) {
            QRectF itemRect;
            if (clipData.points.size() >= 2) {
                // 使用点集的边界矩形
                QRectF rect(clipData.points[0], clipData.points[0]);
                for (const auto& p : clipData.points) {
                    rect.setLeft(std::min(rect.left(), p.x()));
                    rect.setTop(std::min(rect.top(), p.y()));
                    rect.setRight(std::max(rect.right(), p.x()));
                    rect.setBottom(std::max(rect.bottom(), p.y()));
                }
                itemRect = rect;
            } else {
                // 默认大小
                itemRect = QRectF(0, 0, 50, 50);
            }
            
            // 考虑位置
            itemRect.translate(clipData.position);
            
            // 更新总的边界矩形
            if (first) {
                boundingRect = itemRect;
                first = false;
            } else {
                boundingRect = boundingRect.united(itemRect);
            }
        }
        
        // 计算从边界矩形中心到粘贴位置的偏移
        QPointF centerToPos;
        if (!first) { // 如果边界矩形有效
            centerToPos = pos - boundingRect.center();
            Logger::debug(QString("DrawArea::pasteItemsAtPosition: 偏移量 (%1, %2)")
                         .arg(centerToPos.x())
                         .arg(centerToPos.y()));
        }
        
        // 新创建的图形项集合
        QList<QGraphicsItem*> pastedItems;
        
        // 创建所有图形项，但先不添加到场景
        for (const auto& clipData : m_clipboardData) {
            // 调整粘贴位置，使图形集合居中于目标位置
            QPointF pastePos = clipData.position + centerToPos;
            
            Logger::debug(QString("DrawArea::pasteItemsAtPosition: 创建图形项，类型 %1，位置 (%2, %3)")
                         .arg(static_cast<int>(clipData.type))
                         .arg(pastePos.x())
                         .arg(pastePos.y()));
            
            QGraphicsItem* newItem = createItemFromClipboardData(clipData, pastePos);
            
            if (newItem) {
                pastedItems.append(newItem);
            }
        }
        
        // 只有在有图形项创建成功的情况下才继续
        if (!pastedItems.isEmpty()) {
            Logger::debug(QString("DrawArea::pasteItemsAtPosition: 为 %1 个图形项创建粘贴命令")
                         .arg(pastedItems.size()));
                         
            // 1. 首先将所有图形项添加到场景，准备执行命令
            for (QGraphicsItem* item : pastedItems) {
                m_scene->addItem(item);
                item->setSelected(true);
            }
            
            // 2. 创建粘贴命令并执行（通过CommandManager记录到命令栈中）
            PasteGraphicCommand* command = new PasteGraphicCommand(this, pastedItems);
            
            Logger::debug("DrawArea::pasteItemsAtPosition: 执行粘贴命令");
            CommandManager::getInstance().executeCommand(command);
            
            // 通知场景更新
            m_scene->update();
            viewport()->update();
            
            Logger::info(QString("已粘贴 %1 个图形项").arg(pastedItems.size()));
        } else {
            Logger::warning("DrawArea::pasteItemsAtPosition: 没有创建任何图形项");
        }
        
        // 结束命令分组
        CommandManager::getInstance().endCommandGroup();
    }
    catch (const std::exception& e) {
        Logger::error(QString("DrawArea::pasteItemsAtPosition: 异常 - %1").arg(e.what()));
        CommandManager::getInstance().endCommandGroup(); // 确保结束分组
    }
    catch (...) {
        Logger::error("DrawArea::pasteItemsAtPosition: 未知异常");
        CommandManager::getInstance().endCommandGroup(); // 确保结束分组
    }
    
    // 重置粘贴标志
    isPasting = false;
}

// 剪切选中的图形项
void DrawArea::cutSelectedItems() {
    copySelectedItems();
    copyToSystemClipboard(); // 同时复制到系统剪贴板
    deleteSelectedGraphics();
    Logger::info("已剪切图形项");
}

// 复制到系统剪贴板
void DrawArea::copyToSystemClipboard() {
    auto selectedItems = getSelectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    // 序列化选中的图形项
    QByteArray itemData = serializeGraphicItems(selectedItems);
    
    // 创建MIME数据
    QMimeData* mimeData = new QMimeData();
    mimeData->setData(MIME_GRAPHICITEMS, itemData);
    
    // 设置到系统剪贴板
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setMimeData(mimeData);
    
    Logger::info(QString("已复制 %1 个图形项到系统剪贴板").arg(selectedItems.size()));
}

// 从系统剪贴板粘贴
void DrawArea::pasteFromSystemClipboard() {
    if (!canPasteFromClipboard()) {
        return;
    }
    
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    
    if (mimeData->hasFormat(MIME_GRAPHICITEMS)) {
        QByteArray itemData = mimeData->data(MIME_GRAPHICITEMS);
        QList<ClipboardItem> clipItems = deserializeGraphicItems(itemData);
        
        // 保存到内部剪贴板
        m_clipboardData = clipItems;
        
        // 粘贴图形项
        pasteItems();
        
        Logger::info(QString("已从系统剪贴板粘贴 %1 个图形项").arg(clipItems.size()));
    }
}

// 检查剪贴板中是否有可粘贴的内容
bool DrawArea::canPasteFromClipboard() const {
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    
    return mimeData && mimeData->hasFormat(MIME_GRAPHICITEMS);
}

// 处理右键菜单事件
void DrawArea::contextMenuEvent(QContextMenuEvent* event) {
    // 创建并显示上下文菜单
    createContextMenu(event->pos());
    event->accept();
}

// 创建右键菜单
void DrawArea::createContextMenu(const QPoint& pos) {
    QMenu menu(this);
    
    // 获取点击位置的场景坐标
    QPointF scenePos = mapToScene(pos);
    
    // 添加各种操作
    QAction* copyAction = menu.addAction("复制");
    QAction* cutAction = menu.addAction("剪切");
    QAction* pasteAction = menu.addAction("粘贴");
    QAction* pasteHereAction = menu.addAction("粘贴到此处");
    
    // 根据当前状态启用/禁用选项
    bool hasSelectedItems = !getSelectedItems().isEmpty();
    bool canPaste = !m_clipboardData.isEmpty() || canPasteFromClipboard();
    
    copyAction->setEnabled(hasSelectedItems);
    cutAction->setEnabled(hasSelectedItems);
    pasteAction->setEnabled(canPaste);
    pasteHereAction->setEnabled(canPaste);
    
    // 显示菜单并获取用户选择的操作
    QAction* selectedAction = menu.exec(mapToGlobal(pos));
    
    // 执行选择的操作
    if (selectedAction == copyAction) {
        copySelectedItems();
        copyToSystemClipboard();
    } else if (selectedAction == cutAction) {
        cutSelectedItems();
    } else if (selectedAction == pasteAction) {
        if (!m_clipboardData.isEmpty()) {
            pasteItems();
        } else if (canPasteFromClipboard()) {
            pasteFromSystemClipboard();
        }
    } else if (selectedAction == pasteHereAction) {
        if (!m_clipboardData.isEmpty()) {
            pasteItemsAtPosition(scenePos);
        } else if (canPasteFromClipboard()) {
            const QMimeData* mimeData = QApplication::clipboard()->mimeData();
            QByteArray itemData = mimeData->data(MIME_GRAPHICITEMS);
            m_clipboardData = deserializeGraphicItems(itemData);
            pasteItemsAtPosition(scenePos);
        }
    }
}

// 计算智能粘贴位置
QPointF DrawArea::calculateSmartPastePosition() const {
    // 如果有选中的项，使用最后选中项位置加上偏移
    auto selectedItems = getSelectedItems();
    if (!selectedItems.isEmpty()) {
        QGraphicsItem* lastSelected = selectedItems.last();
        return lastSelected->pos() + QPointF(30, 30); // 添加适当的偏移
    }
    
    // 如果没有选中项，尝试使用视图中心
    return getViewCenterScenePos();
}

// 获取视图中心的场景坐标
QPointF DrawArea::getViewCenterScenePos() const {
    QRect viewRect = viewport()->rect();
    QPointF centerInView(viewRect.width() / 2.0, viewRect.height() / 2.0);
    return mapToScene(centerInView.toPoint());
}

// 保存图形项到内部剪贴板
void DrawArea::saveGraphicItemToClipboard(GraphicItem* item) {
    if (!item) return;
    
    ClipboardItem clipData;
    clipData.type = item->getGraphicType();
    clipData.pen = item->getPen();
    clipData.brush = item->getBrush();
    clipData.points = item->getClipboardPoints();
    clipData.position = item->pos();
    clipData.rotation = item->rotation();
    clipData.scale = item->getScale();
    
    m_clipboardData.append(clipData);
}

// 从剪贴板数据创建图形项
QGraphicsItem* DrawArea::createItemFromClipboardData(const ClipboardItem& data, const QPointF& pastePosition) {
    // 创建图形项
    QGraphicsItem* newItem = m_graphicFactory->createCustomItem(data.type, data.points);
    
    if (!newItem) {
        Logger::error("createItemFromClipboardData: 无法创建图形项");
        return nullptr;
    }
    
    // 设置属性
    if (auto* graphicItem = dynamic_cast<GraphicItem*>(newItem)) {
        // 先设置基本属性
        graphicItem->setPen(data.pen);
        graphicItem->setBrush(data.brush);
        
        // 应用旋转
        graphicItem->setRotation(data.rotation);
        
        // 直接设置内部m_scale属性，绕过内部的scale自动平均处理
        // 这种方法可以保证椭圆等不同的图形类型维持原始的XY比例
        QPointF scale = data.scale;
        
        // 为不同形状特殊处理
        if (data.type == Graphic::ELLIPSE) {
            // 椭圆特殊处理：确保直接使用原始缩放值，不要让它被平均
            // 保存原始缩放信息到私有数据区，以便子类可以访问
            graphicItem->setItemData(1001, QVariant::fromValue(scale.x()));
            graphicItem->setItemData(1002, QVariant::fromValue(scale.y()));
            
            // 这里调用自定义的setScale方法
            graphicItem->setScale(scale);
        } else {
            // 其他形状使用正常的缩放设置
            graphicItem->setScale(scale);
        }
        
        // 应用标志
        graphicItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        graphicItem->setFlag(QGraphicsItem::ItemIsMovable, true);
        
        Logger::debug(QString("创建图形项，类型:%1, 缩放:(%2,%3), 位置:(%4,%5)")
                     .arg(Graphic::graphicTypeToString(data.type))
                     .arg(scale.x(), 0, 'f', 3)
                     .arg(scale.y(), 0, 'f', 3)
                     .arg(pastePosition.x())
                     .arg(pastePosition.y()));
    }
    
    // 设置位置
    newItem->setPos(pastePosition);
    
    return newItem;
}

// 序列化图形项到字节数组
QByteArray DrawArea::serializeGraphicItems(const QList<QGraphicsItem*>& items) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    
    // 写入版本信息
    stream << (qint32)1; // 版本号
    
    // 写入图形项数量
    stream << (qint32)items.size();
    
    // 序列化每个图形项
    for (auto item : items) {
        auto* graphicItem = dynamic_cast<GraphicItem*>(item);
        if (!graphicItem) continue;
        
        // 写入图形类型
        stream << (qint32)graphicItem->getGraphicType();
        
        // 写入画笔
        stream << graphicItem->getPen();
        
        // 写入画刷
        stream << graphicItem->getBrush();
        
        // 写入位置
        stream << graphicItem->pos();
        
        // 写入旋转
        stream << (qreal)graphicItem->rotation();
        
        // 写入缩放
        stream << graphicItem->getScale();
        
        // 写入点集
        auto points = graphicItem->getClipboardPoints();
        stream << (qint32)points.size();
        for (const auto& point : points) {
            stream << point;
        }
    }
    
    return data;
}

// 从字节数组反序列化图形项
QList<DrawArea::ClipboardItem> DrawArea::deserializeGraphicItems(const QByteArray& data) {
    QList<ClipboardItem> result;
    QDataStream stream(data);
    
    // 读取版本信息
    qint32 version;
    stream >> version;
    
    if (version != 1) {
        Logger::error(QString("deserializeGraphicItems: 不支持的版本号 %1").arg(version));
        return result;
    }
    
    // 读取图形项数量
    qint32 itemCount;
    stream >> itemCount;
    
    // 反序列化每个图形项
    for (qint32 i = 0; i < itemCount; ++i) {
        ClipboardItem item;
        
        // 读取图形类型
        qint32 type;
        stream >> type;
        item.type = static_cast<Graphic::GraphicType>(type);
        
        // 读取画笔
        stream >> item.pen;
        
        // 读取画刷
        stream >> item.brush;
        
        // 读取位置
        stream >> item.position;
        
        // 读取旋转
        stream >> item.rotation;
        
        // 读取缩放
        stream >> item.scale;
        
        // 读取点集
        qint32 pointCount;
        stream >> pointCount;
        item.points.reserve(pointCount);
        
        for (qint32 j = 0; j < pointCount; ++j) {
            QPointF point;
            stream >> point;
            item.points.push_back(point);
        }
        
        result.append(item);
    }
    
    return result;
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