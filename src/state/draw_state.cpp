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
                    double length = QLineF(m_startPoint, m_currentPoint).length();
                    statusMsg = QString("正在绘制直线: 长度 %1").arg(length, 0, 'f', 1);
                } else if (m_graphicType == Graphic::RECTANGLE) {
                    statusMsg = QString("正在绘制矩形: %1 x %2").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);
                } else if (m_graphicType == Graphic::ELLIPSE) {
                    statusMsg = QString("正在绘制椭圆: %1 x %2").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);
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
    
    // 检查是否已经在创建过程中
    static bool isCreating = false;
    if (isCreating) {
        Logger::warning("DrawState::createFinalItem: 已有创建图形项的命令正在执行，防止重复创建");
        return nullptr;
    }
    
    // 设置标志，防止重入
    isCreating = true;
    
    Logger::debug("DrawState::createFinalItem: 开始创建图形项");
    
    // 准备画笔和画刷
    QPen pen(m_lineColor, m_lineWidth);
    QBrush brush = m_fillMode ? QBrush(m_fillColor) : QBrush(Qt::transparent);
    
    // 创建命令
    std::vector<QPointF> points;
    
    if (m_graphicType == Graphic::BEZIER) {
        // 为Bezier曲线只使用控制点，不计算曲线上的点
        points = m_bezierControlPoints;
        Logger::debug(QString("DrawState::createFinalItem: 贝塞尔曲线控制点数量: %1").arg(points.size()));
        
        // 不再转换为曲线点，直接使用控制点创建贝塞尔曲线
        // BezierGraphicItem 和 BezierDrawStrategy 会负责绘制曲线
    } else {
        // 其他图形使用起点和终点
        points.push_back(m_startPoint);
        points.push_back(m_currentPoint);
        
        // 对于椭圆类型，确保使用与预览相同的标准化矩形
        if (m_graphicType == Graphic::ELLIPSE) {
            // 清除之前添加的点
            points.clear();
            // 使用标准化矩形的左上角和右下角点，与预览完全匹配
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            points.push_back(rect.topLeft());
            points.push_back(rect.bottomRight());
        }
    }
    
    // 确保点数据合法
    if (points.size() < 2) {
        Logger::warning("DrawState::createFinalItem: 点数据不足，无法创建图形");
        isCreating = false;
        return nullptr;
    }
    
    // 检查DrawArea和场景是否有效
    if (!drawArea || !drawArea->scene()) {
        Logger::error("DrawState::createFinalItem: DrawArea或场景无效");
        isCreating = false;
        return nullptr;
    }
    
    try {
        // 开始命令分组，确保这个绘图操作是独立的一个命令
        CommandManager::getInstance().beginCommandGroup();
        
        // 创建命令对象并执行命令
        Logger::debug(QString("DrawState::createFinalItem: 创建命令对象，图形类型: %1").arg(static_cast<int>(m_graphicType)));
        
        // 检查点集合是否合理
        for (const auto& p : points) {
            if (!std::isfinite(p.x()) || !std::isfinite(p.y())) {
                Logger::error("DrawState::createFinalItem: 点位置包含无效值");
                isCreating = false;
                CommandManager::getInstance().endCommandGroup();
                return nullptr;
            }
        }
        
        CreateGraphicCommand* command = new CreateGraphicCommand(
            drawArea, m_graphicType, points, pen, brush);
        
        Logger::debug("DrawState::createFinalItem: 执行命令");
        CommandManager::getInstance().executeCommand(command);
        
        // 获取创建的图形项
        item = command->getCreatedItem();
        
        if (item) {
            Logger::debug(QString("DrawState::createFinalItem: 图形项创建成功，指针: %1")
                        .arg(reinterpret_cast<quintptr>(item)));
            Logger::info(QString("DrawState: 创建了 %1 图形").arg(Graphic::graphicTypeToString(m_graphicType)));
        } else {
            Logger::warning("DrawState::createFinalItem: 图形项创建失败");
        }
        
        // 结束命令分组
        CommandManager::getInstance().endCommandGroup();
    }
    catch (const std::exception& e) {
        Logger::error(QString("DrawState::createFinalItem: 异常 - %1").arg(e.what()));
        CommandManager::getInstance().endCommandGroup(); // 确保结束分组
    }
    catch (...) {
        Logger::error("DrawState::createFinalItem: 未知异常");
        CommandManager::getInstance().endCommandGroup(); // 确保结束分组
    }
    
    // 重置创建标志
    isCreating = false;
    return item;
}

void DrawState::updatePreviewItem(DrawArea* drawArea)
{
    if (!drawArea || !drawArea->scene()) {
        return;
    }

    // 如果未处于绘制状态，则不需要预览
    if (!m_isDrawing) {
        return;
    }

    // 为不同的图形类型创建或更新预览
    switch (m_graphicType) {
        case Graphic::LINE: {
            // 线条预览
            if (!m_previewItem) {
                // 创建新的线条预览
                m_previewItem = new QGraphicsLineItem(m_startPoint.x(), m_startPoint.y(), m_currentPoint.x(), m_currentPoint.y());
                QPen pen(m_lineColor, m_lineWidth);
                pen.setStyle(Qt::DashLine); // 使用虚线表示预览
                static_cast<QGraphicsLineItem*>(m_previewItem)->setPen(pen);
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有线条
                static_cast<QGraphicsLineItem*>(m_previewItem)->setLine(
                    m_startPoint.x(), m_startPoint.y(), m_currentPoint.x(), m_currentPoint.y());
            }
            break;
        }
        case Graphic::RECTANGLE: {
            // 矩形预览
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            
            if (!m_previewItem) {
                // 创建新的矩形预览
                m_previewItem = new QGraphicsRectItem(rect);
                QPen pen(m_lineColor, m_lineWidth);
                pen.setStyle(Qt::DashLine); // 使用虚线表示预览
                static_cast<QGraphicsRectItem*>(m_previewItem)->setPen(pen);
                
                // 如果设置了填充，则应用填充
                if (m_fillMode) {
                    static_cast<QGraphicsRectItem*>(m_previewItem)->setBrush(QBrush(m_fillColor));
                }
                
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有矩形
                static_cast<QGraphicsRectItem*>(m_previewItem)->setRect(rect);
            }
            break;
        }
        case Graphic::ELLIPSE: {
            // 椭圆预览
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            
            if (!m_previewItem) {
                // 创建新的椭圆预览
                m_previewItem = new QGraphicsEllipseItem(rect);
                QPen pen(m_lineColor, m_lineWidth);
                pen.setStyle(Qt::DashLine); // 使用虚线表示预览
                static_cast<QGraphicsEllipseItem*>(m_previewItem)->setPen(pen);
                
                // 如果设置了填充，则应用填充
                if (m_fillMode) {
                    static_cast<QGraphicsEllipseItem*>(m_previewItem)->setBrush(QBrush(m_fillColor));
                }
                
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有椭圆
                static_cast<QGraphicsEllipseItem*>(m_previewItem)->setRect(rect);
            }
            break;
        }
        case Graphic::BEZIER: {
            // 贝塞尔曲线预览
            if (m_bezierControlPoints.size() >= 2) {
                QPainterPath path;

                // 使用策略类统一算法
                BezierDrawStrategy strategy;  // 创建策略对象复用逻辑
                strategy.setColor(m_lineColor);  // 颜色/线宽参数可忽略，预览时仅需计算路径
                strategy.setLineWidth(m_lineWidth);

                // 直接调用策略类的绘制逻辑生成路径
                if (m_bezierControlPoints.size() == 2) {
                    // 两点直线
                    path.moveTo(m_bezierControlPoints[0]);
                    path.lineTo(m_bezierControlPoints[1]);
                } else {
                    // 高阶贝塞尔：复用优化算法
                    path.moveTo(m_bezierControlPoints[0]);
                    
                    // 1. 动态计算步长（直接复用策略类中的逻辑）
                    double totalPolylineLength = 0.0;
                    for (size_t i = 1; i < m_bezierControlPoints.size(); ++i) {
                        totalPolylineLength += QLineF(m_bezierControlPoints[i-1], m_bezierControlPoints[i]).length();
                    }
                    const double densityFactor = 5.0;
                    const int minSteps = 20, maxSteps = 500;
                    int numSteps = std::clamp(static_cast<int>(totalPolylineLength * densityFactor), minSteps, maxSteps);

                    // 2. 使用策略类计算曲线点
                    for (int step = 1; step <= numSteps; ++step) {
                        double t = static_cast<double>(step) / numSteps;
                        QPointF p = strategy.calculateBezierPoint(m_bezierControlPoints, t); 
                        path.lineTo(p);
                    }
                }

                // 更新预览图形项
                if (!m_previewItem) {
                    m_previewItem = new QGraphicsPathItem(path);
                    QPen pen(m_lineColor, m_lineWidth);
                    pen.setStyle(Qt::DashLine);
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setPen(pen);
                    drawArea->scene()->addItem(m_previewItem);
                } else {
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setPath(path);
                }
            }
            break;
        }
        default:
            break;
    }
    
    // 确保场景更新
    drawArea->scene()->update();
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