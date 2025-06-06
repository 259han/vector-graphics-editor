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
                } else if (m_graphicType == Graphic::FLOWCHART_PROCESS) {
                    statusMsg = QString("正在绘制流程图处理框: %1 x %2").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);
                } else if (m_graphicType == Graphic::FLOWCHART_DECISION) {
                    statusMsg = QString("正在绘制流程图判断框: %1 x %2").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);
                } else if (m_graphicType == Graphic::FLOWCHART_START_END) {
                    statusMsg = QString("正在绘制流程图开始/结束框: %1 x %2").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);
                } else if (m_graphicType == Graphic::FLOWCHART_IO) {
                    statusMsg = QString("正在绘制流程图输入/输出框: %1 x %2").arg(width, 0, 'f', 1).arg(height, 0, 'f', 1);
                } else if (m_graphicType == Graphic::FLOWCHART_CONNECTOR) {
                    double length = QLineF(m_startPoint, m_currentPoint).length();
                    statusMsg = QString("正在绘制流程图连接器: 长度 %1").arg(length, 0, 'f', 1);
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
                case Graphic::FLOWCHART_PROCESS:
                    statusMsg = "流程图处理框: 按住左键并拖动鼠标绘制处理框（矩形）";
                    break;
                case Graphic::FLOWCHART_DECISION:
                    statusMsg = "流程图判断框: 按住左键并拖动鼠标绘制判断框（菱形）";
                    break;
                case Graphic::FLOWCHART_START_END:
                    statusMsg = "流程图开始/结束框: 按住左键并拖动鼠标绘制开始/结束框（圆角矩形）";
                    break;
                case Graphic::FLOWCHART_IO:
                    statusMsg = "流程图输入/输出框: 按住左键并拖动鼠标绘制输入/输出框（平行四边形）";
                    break;
                case Graphic::FLOWCHART_CONNECTOR:
                    statusMsg = "流程图连接器: 按住左键并拖动鼠标绘制连接线（带箭头）";
                    break;
                default:
                    statusMsg = "绘图工具: 点击并拖动鼠标进行绘制";
                    break;
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
    QBrush brush;
    
    // 为流程图元素设置默认的白色填充，除非已经设置了填充色
    if (m_graphicType == Graphic::FLOWCHART_PROCESS || 
        m_graphicType == Graphic::FLOWCHART_DECISION || 
        m_graphicType == Graphic::FLOWCHART_START_END || 
        m_graphicType == Graphic::FLOWCHART_IO) {
        if (m_fillMode) {
            brush = QBrush(m_fillColor);
        } else {
            brush = QBrush(Qt::white); // 默认白色填充
        }
        Logger::debug(QString("DrawState::createFinalItem: 设置流程图元素填充色: %1").arg(brush.color().name()));
    } else if (m_graphicType == Graphic::FLOWCHART_CONNECTOR) {
        // 对于连接器，画刷用于箭头填充，应与线条颜色一致
        brush = QBrush(m_lineColor);
        Logger::debug(QString("DrawState::createFinalItem: 设置流程图连接器箭头填充色: %1").arg(brush.color().name()));
    } else {
        // 其他图形类型使用原有的填充规则
        brush = m_fillMode ? QBrush(m_fillColor) : QBrush(Qt::transparent);
    }
    
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
        
        // 对于流程图连接器，确保起点和终点顺序正确
        if (m_graphicType == Graphic::FLOWCHART_CONNECTOR) {
            // 清除之前添加的点
            points.clear();
            // 确保连接器的起点和终点与预览一致
            points.push_back(m_startPoint);
            points.push_back(m_currentPoint);
            
            Logger::debug(QString("DrawState::createFinalItem: 流程图连接器: (%1,%2)-(%3,%4)")
                         .arg(m_startPoint.x()).arg(m_startPoint.y())
                         .arg(m_currentPoint.x()).arg(m_currentPoint.y()));
        }
        // 对于基于矩形的流程图元素，需要特殊处理
        else if (m_graphicType == Graphic::FLOWCHART_PROCESS || 
                 m_graphicType == Graphic::FLOWCHART_DECISION || 
                 m_graphicType == Graphic::FLOWCHART_START_END || 
                 m_graphicType == Graphic::FLOWCHART_IO) {
            // 清除之前添加的点
            points.clear();
            
            // 计算标准化矩形
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            
            // 流程图元素使用矩形中心点，这里我们只保留中心点
            points.push_back(rect.center());
            
            // 为了保留矩形大小，添加边长信息
            // 注意：作为工厂函数的第二个参数，这只用于传递大小信息
            // 我们使用右下角点来保存大小信息
            QPointF sizePoint = rect.center() + QPointF(rect.width()/2, rect.height()/2);
            points.push_back(sizePoint);
            
            Logger::debug(QString("DrawState::createFinalItem: 流程图元素: 中心点(%1,%2) 大小(%3x%4)")
                         .arg(rect.center().x()).arg(rect.center().y())
                         .arg(rect.width()).arg(rect.height()));
        }
        // 对于椭圆类型，确保使用与预览相同的标准化矩形
        else if (m_graphicType == Graphic::ELLIPSE) {
            // 使用与预览相同的标准化矩形
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            points.clear();
            // 使用左上角和右下角点，与预览完全匹配
            points.push_back(rect.topLeft());
            points.push_back(rect.bottomRight());
            
            Logger::debug(QString("DrawState::createFinalItem: 标准化矩形: (%1,%2)-(%3,%4)")
                         .arg(rect.left()).arg(rect.top())
                         .arg(rect.right()).arg(rect.bottom()));
        }
        // 对于矩形类型，使用与预览相同的标准化矩形
        else if (m_graphicType == Graphic::RECTANGLE) {
            // 使用与预览相同的标准化矩形
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            points.clear();
            // 使用左上角和右下角点，与预览完全匹配
            points.push_back(rect.topLeft());
            points.push_back(rect.bottomRight());
            
            Logger::debug(QString("DrawState::createFinalItem: 标准化矩形: (%1,%2)-(%3,%4)")
                         .arg(rect.left()).arg(rect.top())
                         .arg(rect.right()).arg(rect.bottom()));
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
        // 检查点集合是否合理
        for (const auto& p : points) {
            if (!std::isfinite(p.x()) || !std::isfinite(p.y())) {
                Logger::error("DrawState::createFinalItem: 点位置包含无效值");
                isCreating = false;
                return nullptr;
            }
        }
        
        Logger::debug(QString("DrawState::createFinalItem: 创建命令对象，图形类型: %1").arg(static_cast<int>(m_graphicType)));
        
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
            
            // 立即通知DrawArea处理新创建的流程图元素
            if (m_graphicType == Graphic::FLOWCHART_PROCESS || 
                m_graphicType == Graphic::FLOWCHART_DECISION || 
                m_graphicType == Graphic::FLOWCHART_START_END || 
                m_graphicType == Graphic::FLOWCHART_IO) {
                drawArea->handleNewGraphicItem(item);
            }
        } else {
            Logger::warning("DrawState::createFinalItem: 图形项创建失败");
        }
    }
    catch (const std::exception& e) {
        Logger::error(QString("DrawState::createFinalItem: 异常 - %1").arg(e.what()));
    }
    catch (...) {
        Logger::error("DrawState::createFinalItem: 未知异常");
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
        
        // 流程图元素预览
        case Graphic::FLOWCHART_PROCESS: {
            // 处理框（矩形）预览
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
                } else {
                    static_cast<QGraphicsRectItem*>(m_previewItem)->setBrush(QBrush(Qt::white));
                }
                
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有矩形
                static_cast<QGraphicsRectItem*>(m_previewItem)->setRect(rect);
            }
            break;
        }
        
        case Graphic::FLOWCHART_DECISION: {
            // 判断框（菱形）预览
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            
            if (!m_previewItem) {
                // 创建新的路径项
                QPainterPath path;
                path.moveTo(rect.center().x(), rect.top());          // 上
                path.lineTo(rect.right(), rect.center().y());        // 右
                path.lineTo(rect.center().x(), rect.bottom());       // 下
                path.lineTo(rect.left(), rect.center().y());         // 左
                path.closeSubpath();
                
                m_previewItem = new QGraphicsPathItem(path);
                QPen pen(m_lineColor, m_lineWidth);
                pen.setStyle(Qt::DashLine); // 使用虚线表示预览
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPen(pen);
                
                // 如果设置了填充，则应用填充
                if (m_fillMode) {
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setBrush(QBrush(m_fillColor));
                } else {
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setBrush(QBrush(Qt::white));
                }
                
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有菱形
                QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
                QPainterPath path;
                path.moveTo(rect.center().x(), rect.top());          // 上
                path.lineTo(rect.right(), rect.center().y());        // 右
                path.lineTo(rect.center().x(), rect.bottom());       // 下
                path.lineTo(rect.left(), rect.center().y());         // 左
                path.closeSubpath();
                
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPath(path);
            }
            break;
        }
        
        case Graphic::FLOWCHART_START_END: {
            // 开始/结束框（圆角矩形）预览
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            
            if (!m_previewItem) {
                // 创建新的路径项
                QPainterPath path;
                path.addRoundedRect(rect, 15, 15); // 15像素的圆角
                
                m_previewItem = new QGraphicsPathItem(path);
                QPen pen(m_lineColor, m_lineWidth);
                pen.setStyle(Qt::DashLine); // 使用虚线表示预览
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPen(pen);
                
                // 如果设置了填充，则应用填充
                if (m_fillMode) {
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setBrush(QBrush(m_fillColor));
                } else {
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setBrush(QBrush(Qt::white));
                }
                
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有圆角矩形
                QPainterPath path;
                path.addRoundedRect(rect, 15, 15);
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPath(path);
            }
            break;
        }
        
        case Graphic::FLOWCHART_IO: {
            // 输入/输出框（平行四边形）预览
            QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
            
            if (!m_previewItem) {
                // 创建新的路径项
                QPainterPath path;
                // 计算倾斜量，使用宽度和高度的比例来确定合适的值
                // 新的计算方式确保即使在矩形很窄时左边的线也能显示
                qreal skewOffset = qMin(rect.height() * 0.2, rect.width() * 0.3);
                
                QPolygonF parallelogram;
                parallelogram << QPointF(rect.left() + skewOffset, rect.top())    // 左上
                             << QPointF(rect.right(), rect.top())                // 右上
                             << QPointF(rect.right() - skewOffset, rect.bottom()) // 右下
                             << QPointF(rect.left(), rect.bottom());             // 左下
                
                path.addPolygon(parallelogram);
                path.closeSubpath(); // 确保路径闭合
                
                m_previewItem = new QGraphicsPathItem(path);
                QPen pen(m_lineColor, m_lineWidth);
                pen.setStyle(Qt::DashLine); // 使用虚线表示预览
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPen(pen);
                
                // 如果设置了填充，则应用填充
                if (m_fillMode) {
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setBrush(QBrush(m_fillColor));
                } else {
                    static_cast<QGraphicsPathItem*>(m_previewItem)->setBrush(QBrush(Qt::white));
                }
                
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有平行四边形
                QPainterPath path;
                // 使用相同的改进计算方式
                qreal skewOffset = qMin(rect.height() * 0.2, rect.width() * 0.3);
                
                QPolygonF parallelogram;
                parallelogram << QPointF(rect.left() + skewOffset, rect.top())    // 左上
                             << QPointF(rect.right(), rect.top())                // 右上
                             << QPointF(rect.right() - skewOffset, rect.bottom()) // 右下
                             << QPointF(rect.left(), rect.bottom());             // 左下
                
                path.addPolygon(parallelogram);
                path.closeSubpath(); // 确保路径闭合
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPath(path);
            }
            break;
        }
        
        case Graphic::FLOWCHART_CONNECTOR: {
            // 连接器（带箭头的线）预览
            if (!m_previewItem) {
                // 创建新的路径预览（使用QPainterPath而不是简单线段）
                QPainterPath path;
                path.moveTo(m_startPoint);
                path.lineTo(m_currentPoint);
                
                m_previewItem = new QGraphicsPathItem(path);
                QPen pen(m_lineColor, m_lineWidth);
                pen.setStyle(Qt::DashLine); // 使用虚线表示预览
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPen(pen);
                
                // 从DrawArea获取当前设置的箭头类型
                FlowchartConnectorItem::ArrowType arrowType = FlowchartConnectorItem::SingleArrow; // 默认单箭头
                
                // 如果DrawArea可用，获取其设置的箭头类型
                if (drawArea) {
                    arrowType = drawArea->getArrowType();
                }
                
                // 根据箭头类型，添加相应的箭头图形
                // 1. 起点箭头（仅在双箭头模式下添加）
                if (arrowType == FlowchartConnectorItem::DoubleArrow) {
                    QGraphicsPolygonItem* startArrowHead = new QGraphicsPolygonItem(m_previewItem);
                    
                    // 计算箭头角度 - 从终点指向起点
                    double startAngle = std::atan2(m_startPoint.y() - m_currentPoint.y(), 
                                              m_startPoint.x() - m_currentPoint.x());
                    
                    // 计算垂直于线条的方向
                    double startPerpAngle = startAngle + M_PI/2;
                    
                    // 设置箭头大小
                    double arrowWidth = 8.0;
                    double arrowHeight = 12.0;
                    
                    // 计算箭头点
                    QPointF startBaseCenter = m_startPoint - QPointF(cos(startAngle) * arrowHeight/2, sin(startAngle) * arrowHeight/2);
                    QPointF startBaseLeft = startBaseCenter + QPointF(cos(startPerpAngle) * arrowWidth/2, sin(startPerpAngle) * arrowWidth/2);
                    QPointF startBaseRight = startBaseCenter - QPointF(cos(startPerpAngle) * arrowWidth/2, sin(startPerpAngle) * arrowWidth/2);
                    QPointF startTip = startBaseCenter + QPointF(cos(startAngle) * arrowHeight, sin(startAngle) * arrowHeight);
                    
                    QPolygonF startArrowShape;
                    startArrowShape << startTip << startBaseLeft << startBaseRight;
                    startArrowHead->setPolygon(startArrowShape);
                    startArrowHead->setBrush(QBrush(m_lineColor));
                }
                
                // 2. 终点箭头（在单箭头和双箭头模式下添加）
                if (arrowType != FlowchartConnectorItem::NoArrow) {
                    QGraphicsPolygonItem* endArrowHead = new QGraphicsPolygonItem(m_previewItem);
                    
                    // 计算箭头角度 - 从起点指向终点
                    double endAngle = std::atan2(m_currentPoint.y() - m_startPoint.y(), 
                                            m_currentPoint.x() - m_startPoint.x());
                    
                    // 计算垂直于线条的方向
                    double endPerpAngle = endAngle + M_PI/2;
                    
                    // 设置箭头大小
                    double arrowWidth = 8.0;
                    double arrowHeight = 12.0;
                    
                    // 计算箭头点
                    QPointF endBaseCenter = m_currentPoint - QPointF(cos(endAngle) * arrowHeight/2, sin(endAngle) * arrowHeight/2);
                    QPointF endBaseLeft = endBaseCenter + QPointF(cos(endPerpAngle) * arrowWidth/2, sin(endPerpAngle) * arrowWidth/2);
                    QPointF endBaseRight = endBaseCenter - QPointF(cos(endPerpAngle) * arrowWidth/2, sin(endPerpAngle) * arrowWidth/2);
                    QPointF endTip = endBaseCenter + QPointF(cos(endAngle) * arrowHeight, sin(endAngle) * arrowHeight);
                    
                    QPolygonF endArrowShape;
                    endArrowShape << endTip << endBaseLeft << endBaseRight;
                    endArrowHead->setPolygon(endArrowShape);
                    endArrowHead->setBrush(QBrush(m_lineColor));
                }
                
                drawArea->scene()->addItem(m_previewItem);
            } else {
                // 更新现有路径
                QPainterPath path;
                path.moveTo(m_startPoint);
                path.lineTo(m_currentPoint);
                static_cast<QGraphicsPathItem*>(m_previewItem)->setPath(path);
                
                // 从DrawArea获取当前设置的箭头类型
                FlowchartConnectorItem::ArrowType arrowType = FlowchartConnectorItem::SingleArrow; // 默认单箭头
                
                // 如果DrawArea可用，获取其设置的箭头类型
                if (drawArea) {
                    arrowType = drawArea->getArrowType();
                }
                
                // 移除所有现有的箭头
                QList<QGraphicsItem*> children = m_previewItem->childItems();
                for (QGraphicsItem* child : children) {
                    m_previewItem->scene()->removeItem(child);
                    delete child;
                }
                
                // 根据箭头类型，添加相应的箭头图形
                // 1. 起点箭头（仅在双箭头模式下添加）
                if (arrowType == FlowchartConnectorItem::DoubleArrow) {
                    QGraphicsPolygonItem* startArrowHead = new QGraphicsPolygonItem(m_previewItem);
                    
                    // 计算箭头角度 - 从终点指向起点
                    double startAngle = std::atan2(m_startPoint.y() - m_currentPoint.y(), 
                                              m_startPoint.x() - m_currentPoint.x());
                    
                    // 计算垂直于线条的方向
                    double startPerpAngle = startAngle + M_PI/2;
                    
                    // 设置箭头大小
                    double arrowWidth = 8.0;
                    double arrowHeight = 12.0;
                    
                    // 计算箭头点
                    QPointF startBaseCenter = m_startPoint - QPointF(cos(startAngle) * arrowHeight/2, sin(startAngle) * arrowHeight/2);
                    QPointF startBaseLeft = startBaseCenter + QPointF(cos(startPerpAngle) * arrowWidth/2, sin(startPerpAngle) * arrowWidth/2);
                    QPointF startBaseRight = startBaseCenter - QPointF(cos(startPerpAngle) * arrowWidth/2, sin(startPerpAngle) * arrowWidth/2);
                    QPointF startTip = startBaseCenter + QPointF(cos(startAngle) * arrowHeight, sin(startAngle) * arrowHeight);
                    
                    QPolygonF startArrowShape;
                    startArrowShape << startTip << startBaseLeft << startBaseRight;
                    startArrowHead->setPolygon(startArrowShape);
                    startArrowHead->setBrush(QBrush(m_lineColor));
                }
                
                // 2. 终点箭头（在单箭头和双箭头模式下添加）
                if (arrowType != FlowchartConnectorItem::NoArrow) {
                    QGraphicsPolygonItem* endArrowHead = new QGraphicsPolygonItem(m_previewItem);
                    
                    // 计算箭头角度 - 从起点指向终点
                    double endAngle = std::atan2(m_currentPoint.y() - m_startPoint.y(), 
                                            m_currentPoint.x() - m_startPoint.x());
                    
                    // 计算垂直于线条的方向
                    double endPerpAngle = endAngle + M_PI/2;
                    
                    // 设置箭头大小
                    double arrowWidth = 8.0;
                    double arrowHeight = 12.0;
                    
                    // 计算箭头点
                    QPointF endBaseCenter = m_currentPoint - QPointF(cos(endAngle) * arrowHeight/2, sin(endAngle) * arrowHeight/2);
                    QPointF endBaseLeft = endBaseCenter + QPointF(cos(endPerpAngle) * arrowWidth/2, sin(endPerpAngle) * arrowWidth/2);
                    QPointF endBaseRight = endBaseCenter - QPointF(cos(endPerpAngle) * arrowWidth/2, sin(endPerpAngle) * arrowWidth/2);
                    QPointF endTip = endBaseCenter + QPointF(cos(endAngle) * arrowHeight, sin(endAngle) * arrowHeight);
                    
                    QPolygonF endArrowShape;
                    endArrowShape << endTip << endBaseLeft << endBaseRight;
                    endArrowHead->setPolygon(endArrowShape);
                    endArrowHead->setBrush(QBrush(m_lineColor));
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
        case Graphic::FLOWCHART_PROCESS:
            statusMsg = "流程图处理框: 按住左键并拖动鼠标绘制处理框（矩形）";
            break;
        case Graphic::FLOWCHART_DECISION:
            statusMsg = "流程图判断框: 按住左键并拖动鼠标绘制判断框（菱形）";
            break;
        case Graphic::FLOWCHART_START_END:
            statusMsg = "流程图开始/结束框: 按住左键并拖动鼠标绘制开始/结束框（圆角矩形）";
            break;
        case Graphic::FLOWCHART_IO:
            statusMsg = "流程图输入/输出框: 按住左键并拖动鼠标绘制输入/输出框（平行四边形）";
            break;
        case Graphic::FLOWCHART_CONNECTOR:
            statusMsg = "流程图连接器: 按住左键并拖动鼠标绘制连接线（带箭头）";
            break;
        default:
            statusMsg = "绘图工具: 点击并拖动鼠标进行绘制";
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