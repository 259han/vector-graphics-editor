#include "draw_state.h"
#include "../ui/draw_area.h"
#include "../core/graphic_item.h"
#include "../core/graphics_item_factory.h"
#include "../command/create_graphic_command.h"
#include "../command/command_manager.h"
#include <QMouseEvent>
#include <QGraphicsScene>
#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <cmath>
#include <QStack>
#include <QImage>
#include "../core/draw_strategy.h"
#include <QDebug>
#include <QGraphicsItem>
#include <QBrush>
#include <QPen>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include "../utils/logger.h"
#include <QTimer>

DrawState::DrawState(Graphic::GraphicType type)
    : m_graphicType(type)
{
    // 根据图形类型选择适当的绘制策略
    switch (type) {
        case Graphic::LINE:
            m_drawStrategy = std::make_shared<LineDrawStrategy>();
            break;
        case Graphic::RECTANGLE:
            m_drawStrategy = std::make_shared<RectangleDrawStrategy>();
            break;
        case Graphic::CIRCLE:
            m_drawStrategy = std::make_shared<CircleDrawStrategy>();
            break;
        case Graphic::ELLIPSE:
            m_drawStrategy = std::make_shared<EllipseDrawStrategy>();
            break;
        case Graphic::BEZIER:
            m_drawStrategy = std::make_shared<BezierDrawStrategy>();
            break;
        // 其他类型的绘制策略初始化
        /*
        case Graphic::TRIANGLE:
            // 处理三角形
            break;
        */
        default:
            m_drawStrategy = std::make_shared<LineDrawStrategy>();
            break;
    }
    
    // 确保绘制策略使用当前的颜色和线宽
    if (m_drawStrategy) {
        m_drawStrategy->setColor(m_lineColor);
        m_drawStrategy->setLineWidth(m_lineWidth);
    }
    
    Logger::info(QString("DrawState: 创建绘制状态，图形类型: %1").arg(static_cast<int>(type)));
}

void DrawState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event)
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

void DrawState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event)
{
    if (!drawArea || !drawArea->scene()) {
        return;
    }

    QPointF newPos = drawArea->mapToScene(event->pos());
    
    // 只有当位置发生显著变化时才更新
    if (qAbs(newPos.x() - m_currentPoint.x()) > 1 || 
        qAbs(newPos.y() - m_currentPoint.y()) > 1) {
        m_currentPoint = newPos;
        
        // 更新鼠标移动时的提示信息
        QString statusMsg;
        if (m_isDrawing) {
            if (m_graphicType == Graphic::BEZIER) {
                statusMsg = QString("贝塞尔曲线: 点击添加控制点，右键结束绘制");
            } else {
                double width = qAbs(m_currentPoint.x() - m_startPoint.x());
                double height = qAbs(m_currentPoint.y() - m_startPoint.y());
                
                if (m_graphicType == Graphic::LINE) {
                    statusMsg = QString("正在绘制直线: 长度 %.1f").arg(
                        QLineF(m_startPoint, m_currentPoint).length());
                } else if (m_graphicType == Graphic::RECTANGLE) {
                    statusMsg = QString("正在绘制矩形: %.1f x %.1f").arg(width).arg(height);
                } else if (m_graphicType == Graphic::ELLIPSE) {
                    statusMsg = QString("正在绘制椭圆: %.1f x %.1f").arg(width).arg(height);
                }
            }
            updateStatusMessage(drawArea, statusMsg);
            
            // 只有在绘制状态下才更新预览
            updatePreviewItem(drawArea);
        }
    }
}

void DrawState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 对于非Bezier曲线类型，鼠标释放即完成绘制
        if (m_isDrawing && m_graphicType != Graphic::BEZIER) {
            // 移除预览项
            if (m_previewItem) {
                drawArea->scene()->removeItem(m_previewItem);
                delete m_previewItem;
                m_previewItem = nullptr;
            }
            
            // 记录绘制完成时的信息
            QPointF startPoint = m_startPoint;
            QPointF endPoint = m_currentPoint;
            Graphic::GraphicType type = m_graphicType;
            
            // 重置状态变量
            m_isDrawing = false;
            m_bezierControlPoints.clear();
            
            // 创建最终图形
            QGraphicsItem* finalItem = createFinalItem(drawArea);
            
            // 立即更新场景和视图，确保图形显示
            if (drawArea && drawArea->scene()) {
                drawArea->scene()->update();
                drawArea->viewport()->update();
            }
            
            // 更新状态栏消息
            QString statusMsg;
            if (m_graphicType == Graphic::LINE) {
                statusMsg = "直线工具: 按住左键并拖动鼠标绘制直线";
            } else if (m_graphicType == Graphic::RECTANGLE) {
                statusMsg = "矩形工具: 按住左键并拖动鼠标绘制矩形";
            } else if (m_graphicType == Graphic::ELLIPSE) {
                statusMsg = "椭圆工具: 按住左键并拖动鼠标绘制椭圆";
            } else if (m_graphicType == Graphic::BEZIER) {
                statusMsg = "贝塞尔曲线工具: 点击添加控制点, 右键点击完成曲线";
            }
            updateStatusMessage(drawArea, statusMsg);
        }
    }
}

void DrawState::paintEvent(DrawArea* drawArea, QPainter* painter)
{
    // 在QGraphicsScene模式下，绘制由场景自动处理
    // 此方法可能不需要或仅用于特殊效果
}

void DrawState::keyPressEvent(DrawArea* drawArea, QKeyEvent* event)
{
    // 处理键盘事件
    if (event->key() == Qt::Key_Escape) {
        // 取消当前绘制
        if (m_isDrawing) {
            Logger::debug("DrawState::keyPressEvent: 按ESC取消绘制");
            
            // 重置状态
            m_isDrawing = false;
            
            // 移除临时预览项
            if (m_previewItem && drawArea->scene()) {
                drawArea->scene()->removeItem(m_previewItem);
                delete m_previewItem;
                m_previewItem = nullptr;
                Logger::debug("DrawState::keyPressEvent: 移除预览项完成");
            }
            
            // 清除控制点标记
            clearControlPointMarkers(drawArea);
            
            // 清空控制点
            m_bezierControlPoints.clear();
            
            // 使用计时器延迟切换状态，确保当前事件处理完成
            Logger::debug("DrawState::keyPressEvent: 使用计时器延迟切换到编辑状态");
            QTimer::singleShot(0, drawArea, [drawArea, this]() {
                Logger::debug("DrawState::延迟函数(ESC键): 开始切换到编辑状态");
                // 手动调用onExitState确保资源清理
                onExitState(drawArea);
                // 切换到编辑状态
                drawArea->setEditState();
                Logger::debug("DrawState::延迟函数(ESC键): 编辑状态切换完成");
            });
            
            Logger::debug("DrawState::keyPressEvent: 取消绘制完成，等待状态切换");
        }
    }
}

void DrawState::setFillColor(const QColor& color)
{
    m_fillColor = color;
}

void DrawState::setFillMode(bool enabled)
{
    m_fillMode = enabled;
    // 在填充模式下，可能需要更改光标
    QApplication::setOverrideCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void DrawState::resetFillMode()
{
    m_fillMode = false;
    QApplication::restoreOverrideCursor();
}

void DrawState::setLineWidth(int width)
{
    m_lineWidth = width;
    if (m_drawStrategy) {
        m_drawStrategy->setLineWidth(width);
    }
}

void DrawState::setLineColor(const QColor& color)
{
    m_lineColor = color;
    if (m_drawStrategy) {
        m_drawStrategy->setColor(color);
    }
}

QGraphicsItem* DrawState::createFinalItem(DrawArea* drawArea)
{
    QGraphicsItem* item = nullptr;
    
    Logger::debug("DrawState::createFinalItem: 开始创建图形项");
    
    // 准备画笔和画刷
    QPen pen(m_lineColor, m_lineWidth);
    QBrush brush = m_fillMode ? QBrush(m_fillColor) : QBrush(Qt::transparent);
    
    // 创建命令
    std::vector<QPointF> points;
    
    if (m_graphicType == Graphic::BEZIER) {
        // 为Bezier曲线使用所有控制点
        points = m_bezierControlPoints;
        Logger::debug(QString("DrawState::createFinalItem: 贝塞尔曲线控制点数量: %1").arg(points.size()));
        
        // 使用与预览相同的算法计算曲线上的点
        if (points.size() > 2) {
            std::vector<QPointF> curvePoints;
            curvePoints.push_back(points[0]);
            
            const int steps = 100; // 与预览使用相同的步数
            for (int i = 1; i <= steps; ++i) {
                double t = static_cast<double>(i) / steps;
                QPointF point = calculateBezierPoint(t, points);
                curvePoints.push_back(point);
            }
            
            points = curvePoints;
        }
    } else {
        // 其他图形使用起点和终点
        points.push_back(m_startPoint);
        points.push_back(m_currentPoint);
    }
    
    // 确保点数据合法
    if (points.size() < 2) {
        Logger::warning("DrawState::createFinalItem: 点数据不足，无法创建图形");
        return nullptr;
    }
    
    // 检查DrawArea和场景是否有效
    if (!drawArea || !drawArea->scene()) {
        Logger::error("DrawState::createFinalItem: DrawArea或场景无效");
        return nullptr;
    }
    
    // 创建命令对象并执行命令
    Logger::debug(QString("DrawState::createFinalItem: 创建命令对象，图形类型: %1").arg(static_cast<int>(m_graphicType)));
    CreateGraphicCommand* command = new CreateGraphicCommand(
        drawArea, m_graphicType, points, pen, brush);
    
    Logger::debug("DrawState::createFinalItem: 执行命令");
    CommandManager::getInstance().executeCommand(command);
    
    // 获取创建的图形项
    item = command->getCreatedItem();
    if (item) {
        Logger::debug("DrawState::createFinalItem: 图形项创建成功");
    } else {
        Logger::warning("DrawState::createFinalItem: 图形项创建失败");
    }
    
    return item;
}

void DrawState::updatePreviewItem(DrawArea* drawArea)
{
    if (!drawArea || !drawArea->scene()) {
        return;
    }

    // 创建新的预览项
    if (m_graphicType == Graphic::BEZIER && m_bezierControlPoints.size() >= 2) {
        QPainterPath path;
        path.moveTo(m_bezierControlPoints[0]);
        
        // 使用n次贝塞尔曲线
        if (m_bezierControlPoints.size() > 2) {
            // 计算贝塞尔曲线上的点
            const int steps = 100; // 曲线精度
            for (int i = 1; i <= steps; ++i) {
                double t = static_cast<double>(i) / steps;
                QPointF point = calculateBezierPoint(t, m_bezierControlPoints);
                path.lineTo(point);
            }
        } else {
            // 只有两个点时，直接画直线
            path.lineTo(m_bezierControlPoints[1]);
        }

        // 如果预览项不存在，创建新的
        if (!m_previewItem) {
            m_previewItem = new QGraphicsPathItem(path);
            QPen pen(m_lineColor, m_lineWidth);
            pen.setStyle(Qt::DashLine); // 使用虚线作为预览
            static_cast<QGraphicsPathItem*>(m_previewItem)->setPen(pen);
            drawArea->scene()->addItem(m_previewItem);
        } else {
            // 如果预览项已存在，直接更新路径
            static_cast<QGraphicsPathItem*>(m_previewItem)->setPath(path);
        }
    } else if (m_previewItem) {
        // 如果不是贝塞尔曲线模式且有预览项，则移除它
        drawArea->scene()->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;
    }
}

// 计算贝塞尔曲线上的点
QPointF DrawState::calculateBezierPoint(double t, const std::vector<QPointF>& points)
{
    if (points.empty()) {
        return QPointF();
    }
    
    std::vector<QPointF> temp = points;
    int n = temp.size() - 1;
    
    // 使用德卡斯特里奥算法计算贝塞尔曲线上的点
    for (int r = 1; r <= n; ++r) {
        for (int i = 0; i <= n - r; ++i) {
            temp[i] = (1 - t) * temp[i] + t * temp[i + 1];
        }
    }
    
    return temp[0];
}

void DrawState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    if (m_isDrawing && m_graphicType == Graphic::BEZIER) {
        // 完成贝塞尔曲线绘制
        if (m_bezierControlPoints.size() >= 2) {
            // 移除预览项
            if (m_previewItem) {
                drawArea->scene()->removeItem(m_previewItem);
                delete m_previewItem;
                m_previewItem = nullptr;
            }
            
            // 创建最终图形项
            createFinalItem(drawArea);
            
            // 重置状态
            m_isDrawing = false;
            m_bezierControlPoints.clear();
            clearControlPointMarkers(drawArea);
            
            // 更新场景
            if (drawArea && drawArea->scene()) {
                drawArea->scene()->update();
            }
        }
    }
}

void DrawState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    if (!m_isDrawing) {
        // 开始绘制
        m_startPoint = scenePos;
        m_currentPoint = scenePos;
        m_isDrawing = true;
        
        if (m_graphicType == Graphic::BEZIER) {
            m_bezierControlPoints.push_back(scenePos);
            // 绘制第一个控制点
            drawControlPoints(drawArea);
        }
    } else {
        // 继续绘制
        m_currentPoint = scenePos;
        
        if (m_graphicType == Graphic::BEZIER) {
            m_bezierControlPoints.push_back(scenePos);
            // 更新控制点显示
            drawControlPoints(drawArea);
            // 更新预览曲线
            updatePreviewItem(drawArea);
        }
    }
}

// 在场景上绘制控制点标记
void DrawState::drawControlPoints(DrawArea* drawArea)
{
    // 清除旧的控制点标记
    clearControlPointMarkers(drawArea);
    
    // 显示控制点编号
    for (size_t i = 0; i < m_bezierControlPoints.size(); ++i) {
        QPointF point = m_bezierControlPoints[i];
        
        // 创建一个小圆点作为标记
        QGraphicsEllipseItem* marker = new QGraphicsEllipseItem(-3, -3, 6, 6);
        marker->setBrush(QBrush(Qt::red)); // 红色填充
        marker->setPen(QPen(Qt::black, 1)); // 黑色边框
        marker->setPos(point);
        marker->setZValue(1001); // 确保在最上层
        
        // 创建文本项
        QGraphicsTextItem* textItem = new QGraphicsTextItem(QString::number(i + 1));
        textItem->setDefaultTextColor(Qt::black); // 黑色字体更易于看清
        
        // 设置字体
        QFont font = textItem->font();
        font.setBold(true);
        textItem->setFont(font);
        
        // 将文本放在控制点的右上方
        textItem->setPos(2, -10);
        textItem->setParentItem(marker);
        textItem->setZValue(1002); // 确保文本在标记之上
        
        // 添加白色背景，增加文本可读性
        QRectF textRect = textItem->boundingRect();
        QGraphicsRectItem* textBackground = new QGraphicsRectItem(textRect);
        textBackground->setBrush(QBrush(QColor(255, 255, 255, 180))); // 半透明白色
        textBackground->setPen(Qt::NoPen);
        textBackground->setParentItem(marker);
        textBackground->setZValue(1001); // 位于文本下方，标记上方
        textBackground->setPos(2, -10);
        
        // 添加到场景并保存引用
        drawArea->scene()->addItem(marker);
        m_controlPointMarkers.push_back(marker);
    }
}

// 清除控制点标记
void DrawState::clearControlPointMarkers(DrawArea* drawArea)
{
    // 从场景中移除所有控制点标记
    for (QGraphicsEllipseItem* marker : m_controlPointMarkers) {
        drawArea->scene()->removeItem(marker);
        delete marker;
    }
    
    // 清空标记列表
    m_controlPointMarkers.clear();
}

void DrawState::wheelEvent(DrawArea* drawArea, QWheelEvent* event)
{
    // 使用基类的缩放和平移处理
    if (handleZoomAndPan(drawArea, event)) {
        return;
    }
    
    // 如果基类没有处理，则传递给父类的默认处理
    event->ignore();
}

void DrawState::onEnterState(DrawArea* drawArea)
{
    if (!drawArea) {
        Logger::warning("DrawState::onEnterState: 传入的DrawArea为空");
        return;
    }
    
    // 从DrawArea获取当前的线条颜色和宽度
    m_lineColor = drawArea->getLineColor();
    m_lineWidth = drawArea->getLineWidth();
    m_fillColor = drawArea->getFillColor();
    
    // 更新绘制策略的颜色和线宽
    if (m_drawStrategy) {
        m_drawStrategy->setColor(m_lineColor);
        m_drawStrategy->setLineWidth(m_lineWidth);
    }
    
    // 更新状态信息
    Logger::info(QString("DrawState: 进入绘制状态，当前图形类型: %1").arg(static_cast<int>(m_graphicType)));
    
    // 基于状态类型更新状态栏消息
    QString statusMsg;
    switch (m_graphicType) {
        case Graphic::LINE:
            statusMsg = "直线工具: 按住左键并拖动鼠标绘制直线";
            break;
        case Graphic::RECTANGLE:
            statusMsg = "矩形工具: 按住左键并拖动鼠标绘制矩形";
            break;
        case Graphic::ELLIPSE:
            statusMsg = "椭圆工具: 按住左键并拖动鼠标绘制椭圆";
            break;
        case Graphic::BEZIER:
            statusMsg = "贝塞尔曲线工具: 点击添加控制点, 右键点击完成曲线";
            break;
        default:
            statusMsg = "绘制工具: 选择绘制工具并开始绘制";
            break;
    }
    
    updateStatusMessage(drawArea, statusMsg);
    
    // 记录日志信息
    Logger::debug(QString("DrawState: 设置线条颜色为 %1, 线宽为 %2")
        .arg(m_lineColor.name())
        .arg(m_lineWidth));
}

void DrawState::onExitState(DrawArea* drawArea)
{
    Logger::info("DrawState::onExitState: 开始退出绘制状态");
    
    // 重置绘制状态
    m_isDrawing = false;
    
    // 清除任何临时预览项
    if (m_previewItem) {
        Logger::debug("DrawState::onExitState: 移除预览项");
        if (drawArea && drawArea->scene()) {
            drawArea->scene()->removeItem(m_previewItem);
        }
        delete m_previewItem;
        m_previewItem = nullptr;
    }
    
    // 清除贝塞尔曲线的控制点标记
    if (!m_controlPointMarkers.empty()) {
        Logger::debug(QString("DrawState::onExitState: 清除 %1 个控制点标记").arg(m_controlPointMarkers.size()));
        clearControlPointMarkers(drawArea);
    }
    
    // 清空控制点
    if (!m_bezierControlPoints.empty()) {
        Logger::debug(QString("DrawState::onExitState: 清除 %1 个贝塞尔控制点").arg(m_bezierControlPoints.size()));
        m_bezierControlPoints.clear();
    }
    
    // 重置鼠标光标
    Logger::debug("DrawState::onExitState: 重置鼠标光标");
    resetCursor(drawArea);
    
    // 确保场景更新
    if (drawArea && drawArea->scene()) {
        Logger::debug("DrawState::onExitState: 更新场景");
        drawArea->scene()->update();
        drawArea->viewport()->update();
    }
    
    Logger::info("DrawState::onExitState: 绘制状态退出完成");
}

void DrawState::handleMiddleMousePress(DrawArea* drawArea, QPointF scenePos)
{
    Logger::debug("DrawState: 中键点击");
    
    // 中键点击用于在绘图过程中临时切换到视图平移模式
    if (drawArea) {
        drawArea->setDragMode(QGraphicsView::ScrollHandDrag);
        setCursor(drawArea, Qt::ClosedHandCursor);
    }
}

// 工具方法实现
QPointF DrawState::getScenePos(DrawArea* drawArea, QMouseEvent* event)
{
    if (!drawArea) {
        return QPointF();
    }
    return drawArea->mapToScene(event->pos());
}

void DrawState::updateStatusMessage(DrawArea* drawArea, const QString& message)
{
    if (drawArea) {
        QMainWindow* mainWindow = qobject_cast<QMainWindow*>(drawArea->window());
        if (mainWindow) {
            mainWindow->statusBar()->showMessage(message);
        }
    }
}

void DrawState::setCursor(DrawArea* drawArea, Qt::CursorShape shape)
{
    if (drawArea) {
        drawArea->setCursor(shape);
    }
}

void DrawState::resetCursor(DrawArea* drawArea)
{
    if (drawArea) {
        drawArea->setCursor(Qt::ArrowCursor);
    }
}

bool DrawState::handleZoomAndPan(DrawArea* drawArea, QWheelEvent* event)
{
    if (!drawArea) {
        return false;
    }

    // 检查是否按住Ctrl键
    if (event->modifiers() & Qt::ControlModifier) {
        // 缩放
        double scaleFactor = 1.15;
        if (event->angleDelta().y() < 0) {
            scaleFactor = 1.0 / scaleFactor;
        }
        
        drawArea->scale(scaleFactor, scaleFactor);
        return true;
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // 水平滚动
        QScrollBar* hBar = drawArea->horizontalScrollBar();
        hBar->setValue(hBar->value() - event->angleDelta().y());
        return true;
    } else {
        // 垂直滚动
        QScrollBar* vBar = drawArea->verticalScrollBar();
        vBar->setValue(vBar->value() - event->angleDelta().y());
        return true;
    }
}