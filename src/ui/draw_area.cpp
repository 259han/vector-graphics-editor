#include "draw_area.h"
#include "../core/graphic_item.h"
#include "../core/line_graphic_item.h"
#include "../core/rectangle_graphic_item.h"
#include "../core/ellipse_graphic_item.h"
#include "../core/circle_graphic_item.h"
#include "../core/bezier_graphic_item.h"
#include "../state/draw_state.h"
#include "../state/edit_state.h"
#include "../state/fill_state.h"
#include "../core/graphics_item_factory.h"
#include "image_resizer.h"
#include "../utils/logger.h"
#include "../command/command_manager.h"
#include "../command/create_graphic_command.h"
#include "../command/transform_command.h"
#include "../command/move_command.h"
#include "../command/style_change_command.h"
#include "../command/selection_command.h"
#include "../command/paste_command.h"
#include "../utils/performance_monitor.h"

#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QPainter>
#include <QDebug>
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
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDataStream>
#include <QMenu>
#include <QContextMenuEvent>
#include <QElapsedTimer>
#include <QRandomGenerator>

// Define the static MIME type constant
const QString DrawArea::MIME_GRAPHICITEMS = "application/x-claudegraph-items";

DrawArea::DrawArea(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_currentState(nullptr)
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
    
    // 连接选择管理器的信号
    connect(m_selectionManager.get(), &SelectionManager::selectionChanged, this, [this]() {
        viewport()->update();
        emit selectionChanged();
    });
}

DrawArea::~DrawArea()
{
    try {
        qDebug() << "DrawArea: 析构开始";
        
        // 断开所有信号连接
        this->disconnect();
        
        // 安全地清理图像调整器
        qDeleteAll(m_imageResizers);
        m_imageResizers.clear();
        
        // 安全地清除选择，防止在对象析构过程中引用已释放对象
        if (m_selectionManager) {
            m_selectionManager->disconnect();
            m_selectionManager->clearSelection();
        }
        
        // 安全地处理场景
        if (m_scene) {
            // 断开场景的连接
            m_scene->disconnect();
            
            // 清除场景的选择
            m_scene->clearSelection();
            m_scene->clearFocus();
        }
        
        // 确保处理任何挂起的事件
        QApplication::processEvents();
        
        qDebug() << "DrawArea: 析构完成";
    }
    catch (const std::exception& e) {
        qCritical() << "DrawArea::~DrawArea: 异常:" << e.what();
    }
    catch (...) {
        qCritical() << "DrawArea::~DrawArea: 未知异常";
    }
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
    // 性能监控
    PERF_SCOPE(DrawTime);
    
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
    // 性能监控
    PERF_SCOPE(EventTime);
    
    // 1. 检查是否处于平移模式
    if (event->button() == Qt::LeftButton && m_spaceKeyPressed) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    
    // 2. 如果没有状态对象，则直接交给基类处理
    if (!m_currentState) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    
    // 3. 优化的状态事件处理
    QPointF scenePos = mapToScene(event->pos());
    
    // 使用新的handleLeftMousePress/handleRightMousePress接口
    if (event->button() == Qt::LeftButton) {
        m_currentState->handleLeftMousePress(this, scenePos);
    } else if (event->button() == Qt::RightButton) {
        m_currentState->handleRightMousePress(this, scenePos);
    } else {
        // 兼容旧接口
        m_currentState->mousePressEvent(this, event);
    }
    
    // 安排一次更新
    scheduleUpdate();
    
    // 4. 事件已被接受或者当前是绘制状态，则不再传递
    if (event->isAccepted() || dynamic_cast<DrawState*>(m_currentState.get())) {
        return;
    }
    
    // 5. 其他情况交给基类处理
    QGraphicsView::mousePressEvent(event);
}

void DrawArea::mouseMoveEvent(QMouseEvent *event)
{
    // 性能监控
    PERF_SCOPE(EventTime);
    
    // 1. 检查是否处于平移模式
    if (m_isPanning) {
        QPointF delta = mapToScene(event->pos()) - mapToScene(m_lastPanPoint.toPoint());
        m_lastPanPoint = event->pos();
        
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        
        event->accept();
        return;
    }
    
    // 2. 如果没有状态对象，则直接交给基类处理
    if (!m_currentState) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    
    // 3. 调用状态的鼠标移动处理
    m_currentState->mouseMoveEvent(this, event);
    
    // 安排一次更新
    scheduleUpdate();
    
    // 4. 事件已被接受或者当前是绘制状态，则不再传递
    if (event->isAccepted() || dynamic_cast<DrawState*>(m_currentState.get())) {
        return;
    }
    
    // 5. 其他情况交给基类处理
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
    // 检查性能监控是否启用
    bool perfEnabled = isPerformanceMonitorEnabled();
    bool overlayShown = isPerformanceOverlayShown();
    
    // 处理Ctrl键
    if (event->key() == Qt::Key_Control) {
        m_ctrlKeyPressed = true;
        if (m_selectionManager) {
            m_selectionManager->setSelectionMode(SelectionManager::MultiSelection);
        }
    }
    
    // 处理复制、剪切、粘贴的快捷键
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
            case Qt::Key_C: // 复制
                copySelectedItems();
                copyToSystemClipboard();
                event->accept();
                return;
                
            case Qt::Key_X: // 剪切
                cutSelectedItems();
                event->accept();
                return;
                
            case Qt::Key_V: // 粘贴
                if (!m_clipboardData.isEmpty()) {
                    pasteItems();
                } else if (canPasteFromClipboard()) {
                    pasteFromSystemClipboard();
                }
                event->accept();
                return;
                
            case Qt::Key_A: // 全选
                if (m_currentState && m_currentState->getStateType() == EditorState::EditState) {
                    selectAllGraphics();
                    event->accept();
                    return;
                }
                break;
        }
    }
    
    // 组合键: Ctrl+Shift+P 切换性能监控
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_P) {
        enablePerformanceMonitor(!perfEnabled);
        event->accept();
        return;
    }
    
    // 组合键: Ctrl+Shift+O 切换性能覆盖层
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_O) {
        showPerformanceOverlay(!overlayShown);
        event->accept();
        return;
    }
    
    // 以下快捷键仅在性能覆盖层显示时有效
    if (perfEnabled && overlayShown) {
        // 组合键: Ctrl+Shift+1/2/3/4 切换图表模式
        if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
            auto& monitor = PerformanceMonitor::instance();
            
            switch (event->key()) {
                case Qt::Key_1:
                    // 切换为折线图
                    monitor.setChartDisplayMode(PerformanceMonitor::LineChart);
                    Logger::info("性能监控图表模式已切换为: 折线图");
                    viewport()->update();
                    event->accept();
                    return;
                    
                case Qt::Key_2:
                    // 切换为柱状图
                    monitor.setChartDisplayMode(PerformanceMonitor::BarChart);
                    Logger::info("性能监控图表模式已切换为: 柱状图");
                    viewport()->update();
                    event->accept();
                    return;
                    
                case Qt::Key_3:
                    // 切换为面积图
                    monitor.setChartDisplayMode(PerformanceMonitor::AreaChart);
                    Logger::info("性能监控图表模式已切换为: 面积图");
                    viewport()->update();
                    event->accept();
                    return;
                    
                case Qt::Key_4:
                    // 切换为散点图
                    monitor.setChartDisplayMode(PerformanceMonitor::DotChart);
                    Logger::info("性能监控图表模式已切换为: 散点图");
                    viewport()->update();
                    event->accept();
                    return;
                    
                case Qt::Key_Up:
                    // 增加透明度
                    {
                        double opacity = monitor.getOverlayOpacity();
                        monitor.setOverlayOpacity(opacity + 0.05);
                        viewport()->update();
                        event->accept();
                        return;
                    }
                    
                case Qt::Key_Down:
                    // 减少透明度
                    {
                        double opacity = monitor.getOverlayOpacity();
                        monitor.setOverlayOpacity(opacity - 0.05);
                        viewport()->update();
                        event->accept();
                        return;
                    }
                    
                case Qt::Key_M:
                    // 切换位置
                    {
                        auto position = monitor.getOverlayPosition();
                        // 循环切换位置
                        position = static_cast<PerformanceMonitor::OverlayPosition>((position + 1) % 4);
                        monitor.setOverlayPosition(position);
                        
                        QString posText;
                        switch (position) {
                            case PerformanceMonitor::TopLeft:
                                posText = "左上角";
                                break;
                            case PerformanceMonitor::TopRight:
                                posText = "右上角";
                                break;
                            case PerformanceMonitor::BottomLeft:
                                posText = "左下角";
                                break;
                            case PerformanceMonitor::BottomRight:
                                posText = "右下角";
                                break;
                        }
                        
                        Logger::info(QString("性能监控覆盖层位置已切换为: %1").arg(posText));
                        viewport()->update();
                        event->accept();
                        return;
                    }
            }
        }
    }
    
    // 如果没有处理，传递给基类
    QGraphicsView::keyPressEvent(event);
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
    
    // 创建QGraphicsPixmapItem并添加到场景
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    if (m_scene) {
        m_scene->addItem(item);
        item->setPos(0, 0);
    }
    
    // 更新视图
    viewport()->update();
}

void DrawArea::saveImage()
{
    // 打开文件对话框选择保存位置
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("保存图像"),
        QString(),
        tr("PNG文件 (*.png);;JPEG文件 (*.jpg);;BMP文件 (*.bmp);;所有文件 (*)")
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // 获取当前场景内容
    QRectF sceneRect = m_scene->itemsBoundingRect();
    QPixmap pixmap(sceneRect.size().toSize());
    pixmap.fill(Qt::white);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter, QRectF(), sceneRect);
    painter.end();
    
    // 保存图像
    if (!pixmap.save(fileName)) {
        QMessageBox::warning(this, tr("保存失败"), tr("无法保存图像到文件: %1").arg(fileName));
    }
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
        // 直接加载图像并添加到场景
        QImage image(fileName);
        if (!image.isNull()) {
            // 将图像添加到场景中心
            QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
            if (m_scene) {
                m_scene->addItem(item);
                
                // 放置在视图中心
                QRectF viewRect = mapToScene(viewport()->rect()).boundingRect();
                QPointF center = viewRect.center();
                QRectF itemRect = item->boundingRect();
                item->setPos(center - QPointF(itemRect.width()/2, itemRect.height()/2));
            }
            
            // 更新视图
            viewport()->update();
        }
    }
}

void DrawArea::moveSelectedGraphics(const QPointF& offset)
{
    // 性能监控
    PERF_SCOPE(LogicTime);
    
    // 使用SelectionManager移动所选图形
    m_selectionManager->moveSelection(offset);
    
    // 确保刷新
    viewport()->update();
}

void DrawArea::rotateSelectedGraphics(double angle)
{
    // 性能监控
    PERF_SCOPE(LogicTime);
    
    if (!m_selectionManager || m_selectionManager->getSelectedItems().isEmpty()) {
        return;
    }
    
    QList<QGraphicsItem*> selectedItems = m_selectionManager->getSelectedItems();
    
    // 获取选择中心点作为旋转中心
    QPointF center = m_selectionManager->selectionCenter();
    
    // 创建旋转命令并执行
    TransformCommand* command = TransformCommand::createRotateCommand(selectedItems, angle, center);
    CommandManager::getInstance().executeCommand(command);
    
    // 更新视图
    viewport()->update();
}

void DrawArea::scaleSelectedGraphics(double factor)
{
    // 性能监控
    PERF_SCOPE(LogicTime);
    
    if (!m_selectionManager || m_selectionManager->getSelectedItems().isEmpty()) {
        return;
    }
    
    QList<QGraphicsItem*> selectedItems = m_selectionManager->getSelectedItems();
    
    // 获取选择中心点作为缩放中心
    QPointF center = m_selectionManager->selectionCenter();
    
    // 创建缩放命令并执行
    TransformCommand* command = TransformCommand::createScaleCommand(selectedItems, factor, center);
    CommandManager::getInstance().executeCommand(command);
    
    // 更新视图
    viewport()->update();
}

void DrawArea::deleteSelectedGraphics()
{
    // 性能监控
    PERF_SCOPE(LogicTime);
    
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

// 选择所有图形
void DrawArea::selectAllGraphics()
{
    // 性能监控
    PERF_SCOPE(LogicTime);
    
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
    QPointF pastePosition = calculateSmartPastePosition();
    
    // 检查剪贴板是否为空
    if (m_clipboardData.isEmpty() || !m_scene) {
        Logger::warning("DrawArea::pasteItems: 剪贴板为空或场景无效，无法粘贴");
        return;
    }
    
    Logger::debug(QString("DrawArea::pasteItems: 开始粘贴 %1 个项目").arg(m_clipboardData.size()));
    
    // 取消当前所有选择
    if (m_selectionManager) {
        m_selectionManager->clearSelection();
    }
    
    // 开始命令组
    CommandManager::getInstance().beginCommandGroup();
    
    // 新创建的图形项集合
    QList<QGraphicsItem*> pastedItems;
    
    try {
        // 为每个剪贴板项创建图形项
        for (const auto& clipData : m_clipboardData) {
            // 创建图形项
            QGraphicsItem* item = m_graphicFactory->createCustomItem(clipData.type, clipData.points);
            if (!item) continue;
            
            GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
            if (!graphicItem) {
                delete item;
                continue;
            }
            
            // 设置图形项属性
            graphicItem->setPen(clipData.pen);
            graphicItem->setBrush(clipData.brush);
            
            // 计算相对位置 - 保持所有粘贴项之间的相对位置
            QPointF relativePos = clipData.position - m_clipboardData.first().position;
            graphicItem->setPos(pastePosition + relativePos);
            
            graphicItem->setRotation(clipData.rotation);
            graphicItem->setScale(clipData.scale);
            graphicItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
            graphicItem->setFlag(QGraphicsItem::ItemIsMovable, true);
            
            // 创建添加图形命令并执行
            CommandManager::getInstance().addCommandToGroup(
                new CreateGraphicCommand(m_scene, graphicItem));
            
            pastedItems.append(graphicItem);
        }
        
        // 结束并提交命令组
        CommandManager::getInstance().commitCommandGroup();
        
        // 选择新粘贴的图形
        for (QGraphicsItem* item : pastedItems) {
            item->setSelected(true);
        }
        
        // 更新视图
        viewport()->update();
        
        Logger::info(QString("DrawArea::pasteItems: 已粘贴 %1 个图形项").arg(pastedItems.size()));
    }
    catch (const std::exception& e) {
        Logger::error(QString("DrawArea::pasteItems: 异常 - %1").arg(e.what()));
        CommandManager::getInstance().endCommandGroup();
    }
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
void DrawArea::contextMenuEvent(QContextMenuEvent* event)
{
    // 创建并显示上下文菜单
    createContextMenu(event->pos());
    event->accept();
}

// 创建右键菜单
void DrawArea::createContextMenu(const QPoint& pos)
{
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
QPointF DrawArea::calculateSmartPastePosition() const
{
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
QPointF DrawArea::getViewCenterScenePos() const
{
    QRect viewRect = viewport()->rect();
    QPointF centerInView(viewRect.width() / 2.0, viewRect.height() / 2.0);
    return mapToScene(centerInView.toPoint());
}

// 保存图形项到内部剪贴板
void DrawArea::saveGraphicItemToClipboard(GraphicItem* item)
{
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
QGraphicsItem* DrawArea::createItemFromClipboardData(const ClipboardItem& data, const QPointF& pastePosition)
{
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
        
        // 处理缩放 - 注意不同图形类型的特殊需求
        QPointF scale = data.scale;
        
        // 所有图形类型都使用统一的缩放设置方法
        // 对于椭圆类等特殊形状，它们已经重写了setScale方法来适当处理
        graphicItem->setScale(scale);
        
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
QByteArray DrawArea::serializeGraphicItems(const QList<QGraphicsItem*>& items)
{
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
QList<DrawArea::ClipboardItem> DrawArea::deserializeGraphicItems(const QByteArray& data)
{
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

void DrawArea::clearSelection()
{
    // 安全地清除选择
    try {
        if (m_selectionManager) {
            m_selectionManager->clearSelection();
        }
        
        // 确保场景中的项目也被清除选择状态
        if (m_scene) {
            m_scene->clearSelection();
        }
    }
    catch (const std::exception& e) {
        qCritical() << "DrawArea::clearSelection: 异常:" << e.what();
    }
}

// 实现drawForeground用于绘制选择控制点
void DrawArea::drawForeground(QPainter *painter, const QRectF &rect)
{
    // 性能监控
    PERF_SCOPE(DrawTime);
    
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

// 添加setHighQualityRendering实现
void DrawArea::setHighQualityRendering(bool enable)
{
    if (m_highQualityRendering != enable) {
        m_highQualityRendering = enable;
        
        // 设置抗锯齿和平滑变换
        setRenderHint(QPainter::Antialiasing, enable);
        setRenderHint(QPainter::SmoothPixmapTransform, enable);
        
        // 根据渲染质量设置不同的视图更新模式
        if (enable) {
            setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        } else {
            setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
        }
        
        // 更新视图
        viewport()->update();
        
        Logger::info(QString("高质量渲染已%1").arg(enable ? "启用" : "禁用"));
    }
}

// 实现性能监控相关方法
void DrawArea::enablePerformanceMonitor(bool enable)
{
    // 防止重入，避免死锁
    static bool isProcessing = false;
    if (isProcessing) {
        Logger::warning("性能监控状态切换正在进行中，忽略重复请求");
        return;
    }
    isProcessing = true;
    
    try {
        // 获取性能监控实例并设置状态
        PerformanceMonitor& monitor = PerformanceMonitor::instance();
        monitor.setEnabled(enable);
        
        // 断开之前可能存在的连接，避免重复连接
        disconnect(&monitor, &PerformanceMonitor::enabledChanged, this, nullptr);
        disconnect(&monitor, &PerformanceMonitor::overlayEnabledChanged, this, nullptr);
        
        // 连接状态变更信号到视图更新
        connect(&monitor, &PerformanceMonitor::enabledChanged,
                this, [this](bool enabled) {
                    viewport()->update();
                    Logger::info(QString("性能监控状态已变更为%1").arg(enabled ? "启用" : "禁用"));
                });
        
        connect(&monitor, &PerformanceMonitor::overlayEnabledChanged,
                this, [this](bool enabled) {
                    viewport()->update();
                });
        
        // 如果启用，生成一些示例数据
        if (enable) {
            // 使用轻量级宏产生示例数据
            for (int i = 0; i < 5; i++) {
                // 模拟各种操作的性能数据
                PERF_START(UpdateTime);
                QThread::msleep(5);
                PERF_END(UpdateTime);
                
                PERF_START(EventTime);
                QThread::msleep(3);
                PERF_END(EventTime);
                
                PERF_START(DrawTime);
                QThread::msleep(7);
                PERF_END(DrawTime);
                
                PERF_START(LogicTime);
                QThread::msleep(4);
                PERF_END(LogicTime);
                
                // 记录一些自定义事件
                PERF_EVENT("InitSample", i);
                
                // 通知帧完成
                PERF_FRAME_COMPLETED();
            }
            
            Logger::info("性能监控已启用，并生成了初始示例数据");
        } else {
            Logger::info("性能监控已禁用");
        }
    }
    catch (const std::exception& e) {
        Logger::error(QString("请求启用性能监控时发生异常: %1").arg(e.what()));
    }
    catch (...) {
        Logger::error("请求启用性能监控时发生未知异常");
    }
    
    isProcessing = false;
}

bool DrawArea::isPerformanceMonitorEnabled() const
{
    // 首先检查实例是否已创建
    if (!PerformanceMonitor::isInstanceCreated()) {
        return false;
    }
    
    try {
        // 实例已创建，获取启用状态
        return PerformanceMonitor::instance().isEnabled();
    } catch (...) {
        // 如果发生异常，则认为未启用
        return false;
    }
}

void DrawArea::showPerformanceOverlay(bool show)
{
    // 防止重入，避免死锁
    static bool isProcessing = false;
    if (isProcessing) {
        Logger::warning("性能覆盖层状态切换正在进行中，忽略重复请求");
        return;
    }
    isProcessing = true;
    
    try {
        // 检查性能监控是否已初始化且已启用
        bool monitorEnabled = isPerformanceMonitorEnabled();
        
        // 如果要显示覆盖层但性能监控未启用，先启用性能监控
        if (show && !monitorEnabled) {
            Logger::info("需要先启用性能监控才能显示覆盖层");
            // 先启用性能监控 - 这会初始化PerformanceMonitor实例
            enablePerformanceMonitor(true);
        }
        
        // 使用引用获取单例实例
        try {
            // 设置覆盖层状态 - 在异步设计中这是非阻塞操作
            PerformanceMonitor::instance().setOverlayEnabled(show);
            // 记录请求日志
            Logger::info(QString("性能覆盖层%1请求已发送").arg(show ? "显示" : "隐藏"));
        } catch (const std::exception& e) {
            Logger::error(QString("设置性能覆盖层状态失败: %1").arg(e.what()));
        }
    }
    catch (const std::exception& e) {
        Logger::error(QString("请求设置性能覆盖层时发生异常: %1").arg(e.what()));
    }
    catch (...) {
        Logger::error("请求设置性能覆盖层时发生未知异常");
    }
    
    isProcessing = false;
}

bool DrawArea::isPerformanceOverlayShown() const
{
    // 首先检查实例是否已创建
    if (!PerformanceMonitor::isInstanceCreated()) {
        return false;
    }
    
    try {
        // 实例已创建，获取覆盖层状态
        return PerformanceMonitor::instance().isOverlayEnabled();
    } catch (...) {
        // 如果发生异常，则认为未启用
        return false;
    }
}

QString DrawArea::getPerformanceReport() const
{
    try {
        // 使用引用获取单例实例，如果尚未初始化会抛出异常
        return PerformanceMonitor::instance().getPerformanceReport();
    } catch (const std::exception& e) {
        // 如果获取实例失败，返回错误消息
        return QString("无法获取性能报告: %1").arg(e.what());
    } catch (...) {
        // 处理未知异常
        return "无法获取性能报告: 未知错误";
    }
}

// 添加paintEvent实现，以支持性能覆盖层
void DrawArea::paintEvent(QPaintEvent *event)
{
    // 整体绘制时间测量
    PERF_SCOPE(DrawTime);
    
    // 记录场景中的图形项数量和复杂度
    if (m_scene) {
        const QList<QGraphicsItem*> sceneItems = m_scene->items();
        PERF_EVENT("ShapesPerFrame", sceneItems.count());
        
        // 计算帧复杂度 - 基于场景物体类型和数量
        int frameComplexity = 0;
        for (QGraphicsItem* item : sceneItems) {
            switch (item->type()) {
                case QGraphicsPixmapItem::Type: frameComplexity += 10; break; // 位图较复杂
                case QGraphicsPathItem::Type: frameComplexity += 5; break;    // 路径中等复杂度
                case QGraphicsLineItem::Type: frameComplexity += 1; break;    // 线条简单
                default: frameComplexity += 2; break;                          // 默认中低复杂度
            }
            
            // 变换和透明度会增加复杂度
            if (!item->transform().isIdentity()) frameComplexity += 3;
            if (item->opacity() < 1.0) frameComplexity += 2;
        }
        
        PERF_EVENT("FrameComplexity", frameComplexity);
    }
    
    // 渲染准备阶段
    {
        PERF_SCOPE(RenderPrepTime);
        // 此处可以添加渲染前的准备工作
    }
    
    // 实际图形绘制阶段
    {
        PERF_SCOPE(ShapesDrawTime);
        QGraphicsView::paintEvent(event);
    }
    
    // 路径处理时间（如有自定义路径处理）
    if (m_scene && !m_scene->items().isEmpty()) {
        PERF_SCOPE(PathProcessTime);
        // 此处可添加额外的路径处理代码
    }
    
    // 渲染性能覆盖层（如果启用）
    if (isPerformanceMonitorEnabled() && isPerformanceOverlayShown()) {
        QPainter painter(viewport());
        QRectF viewRect = QRectF(0, 0, viewport()->width(), viewport()->height());
        PerformanceMonitor::instance().renderOverlay(&painter, viewRect);
    }
    
    // 记录可见区域统计信息
    if (m_scene) {
        // 记录当前视口可见区域大小
        QRectF visibleRect = mapToScene(viewport()->rect()).boundingRect();
        PERF_EVENT("VisibleArea", visibleRect.width() * visibleRect.height());
        
        // 计算可见项目数量
        int visibleItems = 0;
        for (QGraphicsItem* item : m_scene->items(visibleRect)) {
            if (item->isVisible()) visibleItems++;
        }
        PERF_EVENT("VisibleItems", visibleItems);
    }
    
    // 通知帧完成
    PERF_FRAME_COMPLETED();
}

// 修改scheduleUpdate方法，以便在更新过程中进行性能监控
void DrawArea::scheduleUpdate()
{
    if (!m_updatePending) {
        m_updatePending = true;
        QTimer::singleShot(0, this, [this]() {
            if (m_updatePending) {
                m_updatePending = false;
                
                // 更新时间测量
                PERF_SCOPE(UpdateTime);
                
                // 记录场景项目数量
                if (m_scene) {
                    PERF_EVENT("SceneItemsCount", m_scene->items().count());
                    
                    // 记录选中项数量
                    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
                    PERF_EVENT("SelectedItemsCount", selectedItems.count());
                    
                    // 记录项目类型分布
                    int pathItems = 0, pixmapItems = 0, textItems = 0;
                    for (QGraphicsItem* item : m_scene->items()) {
                        if (item->type() == QGraphicsPathItem::Type) pathItems++;
                        else if (item->type() == QGraphicsPixmapItem::Type) pixmapItems++;
                        else if (item->type() == QGraphicsTextItem::Type) textItems++;
                    }
                    PERF_EVENT("PathItemsCount", pathItems);
                    PERF_EVENT("PixmapItemsCount", pixmapItems);
                    PERF_EVENT("TextItemsCount", textItems);
                }
                
                update();
            }
        });
    }
}

// 实现拖放事件处理方法
void DrawArea::dragEnterEvent(QDragEnterEvent *event)
{
    // 添加安全检查
    if (!event || !event->mimeData()) {
        QGraphicsView::dragEnterEvent(event);
        return;
    }
    
    try {
        // 检查是否包含图像数据
        if (event->mimeData()->hasImage() || event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        } else {
            // 如果不接受这种数据，则调用基类方法
            QGraphicsView::dragEnterEvent(event);
        }
    } catch (...) {
        // 出现任何异常，都调用基类方法并拒绝拖放
        QGraphicsView::dragEnterEvent(event);
        event->ignore();
    }
}

void DrawArea::dragMoveEvent(QDragMoveEvent *event)
{
    // 添加安全检查
    if (!event || !event->mimeData()) {
        QGraphicsView::dragMoveEvent(event);
        return;
    }
    
    try {
        if (event->mimeData()->hasImage() || event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        } else {
            // 如果不接受这种数据，则调用基类方法
            QGraphicsView::dragMoveEvent(event);
        }
    } catch (...) {
        // 出现任何异常，都调用基类方法并拒绝拖放
        QGraphicsView::dragMoveEvent(event);
        event->ignore();
    }
}

void DrawArea::dropEvent(QDropEvent *event)
{
    // 使用PERF_SCOPE跟踪拖放处理时间
    PERF_SCOPE(EventTime);
    
    // 添加安全检查
    if (!event || !event->mimeData() || !m_scene) {
        QGraphicsView::dropEvent(event);
        return;
    }
    
    try {
        // 处理图像拖放
        if (event->mimeData()->hasImage()) {
            QImage image = qvariant_cast<QImage>(event->mimeData()->imageData());
            if (!image.isNull()) {
                // 记录图像大小信息
                PERF_EVENT("DroppedImageSize", image.width() * image.height());
                
                // 在拖放位置添加图像
                QPointF pos = event->position();
                importImageAt(image, pos.toPoint());
                event->acceptProposedAction();
                return;
            }
        }
        // 处理文件拖放
        else if (event->mimeData()->hasUrls()) {
            QList<QUrl> urls = event->mimeData()->urls();
            
            // 记录拖放的URL数量
            PERF_EVENT("DroppedUrlCount", urls.size());
            
            for (const QUrl &url : urls) {
                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile();
                    
                    // 检查文件是否存在
                    if (!QFile::exists(filePath)) {
                        continue;
                    }
                    
                    QFileInfo fileInfo(filePath);
                    
                    // 检查是否为图像文件
                    QStringList supportedFormats;
                    for (const QByteArray &format : QImageReader::supportedImageFormats()) {
                        supportedFormats << QString(format);
                    }
                    
                    if (supportedFormats.contains(fileInfo.suffix().toLower())) {
                        // 尝试加载图像
                        QImage image(filePath);
                        if (!image.isNull()) {
                            // 记录文件大小
                            PERF_EVENT("DroppedFileSize", fileInfo.size());
                            
                            // 在拖放位置添加图像
                            QPointF pos = event->position();
                            importImageAt(image, pos.toPoint());
                            event->acceptProposedAction();
                            return; // 只处理第一个图像文件
                        }
                    }
                }
            }
        }
        
        // 如果走到这里，说明我们没有处理这个拖放事件，调用基类方法
        QGraphicsView::dropEvent(event);
    } catch (const std::exception& e) {
        qWarning() << "Exception in dropEvent:" << e.what();
        QGraphicsView::dropEvent(event);
    } catch (...) {
        qWarning() << "Unknown exception in dropEvent";
        QGraphicsView::dropEvent(event);
    }
}

// 辅助方法，在指定位置导入图像
void DrawArea::importImageAt(const QImage &image, const QPoint &pos)
{
    // 使用作用域性能监控
    PERF_SCOPE(LogicTime);
    
    if (image.isNull() || !m_scene) return;
    
    try {
        // 转换坐标从视口到场景
        QPointF scenePos = mapToScene(pos);

        // 开始记录图像处理时间
        PERF_START(EventTime);
        
        // 创建图形项
        QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
        item->setPos(scenePos);
        
        // 记录图像处理结束
        PERF_END(EventTime);
        
        // 开始记录场景更新时间
        PERF_START(UpdateTime);
        
        // 添加到场景
        m_scene->addItem(item);
        
        // 记录场景更新结束
        PERF_END(UpdateTime);
        
        // 记录导入图像的大小
        PERF_EVENT("ImportedImageSize", image.width() * image.height());
        PERF_EVENT("ImportedImageWidth", image.width());
        PERF_EVENT("ImportedImageHeight", image.height());
        
        // 确保视图更新
        viewport()->update();
    } catch (const std::exception& e) {
        qWarning() << "Exception in importImageAt:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception in importImageAt";
    }
}

void DrawArea::flipSelectedGraphics(bool horizontal)
{
    // 性能监控
    PERF_SCOPE(LogicTime);
    
    if (!m_selectionManager || m_selectionManager->getSelectedItems().isEmpty()) {
        return;
    }
    
    QList<QGraphicsItem*> selectedItems = m_selectionManager->getSelectedItems();
    
    // 获取选择中心点作为翻转中心
    QPointF center = m_selectionManager->selectionCenter();
    
    // 创建翻转命令并执行
    TransformCommand* command = TransformCommand::createFlipCommand(selectedItems, horizontal, center);
    CommandManager::getInstance().executeCommand(command);
    
    // 更新视图
    viewport()->update();
    
    // 发出选择变更信号，以便更新UI状态
    emit selectionChanged();
}

// 在指定位置粘贴图形项
void DrawArea::pasteItemsAtPosition(const QPointF& pos) {
    // 检查剪贴板是否为空
    if (m_clipboardData.isEmpty() || !m_scene) {
        Logger::warning("DrawArea::pasteItemsAtPosition: 剪贴板为空或场景无效，无法粘贴");
        return;
    }
    
    Logger::debug(QString("DrawArea::pasteItemsAtPosition: 开始粘贴 %1 个项目到位置 (%2, %3)")
                 .arg(m_clipboardData.size())
                 .arg(pos.x())
                 .arg(pos.y()));
    
    // 取消当前所有选择
    if (m_selectionManager) {
        m_selectionManager->clearSelection();
    }
    
    // 开始命令组
    CommandManager::getInstance().beginCommandGroup();
    
    // 新创建的图形项集合
    QList<QGraphicsItem*> pastedItems;
    
    try {
        // 为每个剪贴板项创建图形项
        for (const auto& clipData : m_clipboardData) {
            // 创建图形项
            QGraphicsItem* item = m_graphicFactory->createCustomItem(clipData.type, clipData.points);
            if (!item) continue;
            
            GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item);
            if (!graphicItem) {
                delete item;
                continue;
            }
            
            // 设置图形项属性
            graphicItem->setPen(clipData.pen);
            graphicItem->setBrush(clipData.brush);
            
            // 计算相对位置 - 保持所有粘贴项之间的相对位置
            QPointF relativePos = clipData.position - m_clipboardData.first().position;
            graphicItem->setPos(pos + relativePos);
            
            graphicItem->setRotation(clipData.rotation);
            graphicItem->setScale(clipData.scale);
            graphicItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
            graphicItem->setFlag(QGraphicsItem::ItemIsMovable, true);
            
            // 创建添加图形命令并执行
            CommandManager::getInstance().addCommandToGroup(
                new CreateGraphicCommand(m_scene, graphicItem));
            
            pastedItems.append(graphicItem);
        }
        
        // 结束并提交命令组
        CommandManager::getInstance().commitCommandGroup();
        
        // 选择新粘贴的图形
        for (QGraphicsItem* item : pastedItems) {
            item->setSelected(true);
        }
        
        // 更新视图
        viewport()->update();
        
        Logger::info(QString("DrawArea::pasteItemsAtPosition: 已粘贴 %1 个图形项").arg(pastedItems.size()));
    }
    catch (const std::exception& e) {
        Logger::error(QString("DrawArea::pasteItemsAtPosition: 异常 - %1").arg(e.what()));
        CommandManager::getInstance().endCommandGroup();
    }
}