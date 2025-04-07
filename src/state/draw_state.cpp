#include "draw_state.h"
#include "../ui/draw_area.h"
#include "../core/graphic_item.h"
#include "../core/graphics_item_factory.h"
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
    
    if (m_drawStrategy) {
        m_drawStrategy->setColor(m_lineColor);
        m_drawStrategy->setLineWidth(m_lineWidth);
    }
}

void DrawState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event)
{
    if (!drawArea) return;
    
    QPointF scenePos = drawArea->mapToScene(event->pos());
    
    // 处理鼠标点击
    if (event->button() == Qt::LeftButton) {
        handleLeftMousePress(drawArea, scenePos);
    } else if (event->button() == Qt::RightButton) {
        handleRightMousePress(drawArea, scenePos);
    }
    
    // 接受此事件
    event->accept();
}

void DrawState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event)
{
    QPointF scenePos = drawArea->mapToScene(event->pos());
    
    if (m_isDrawing) {
        m_currentPoint = scenePos;
        
        // 更新预览图形
        updatePreviewItem(drawArea);
    }
    
    // 更新状态栏信息
    QWidget* topLevelWidget = drawArea->window();
    if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
        QString statusMsg;
        
        if (m_graphicType == Graphic::BEZIER) {
            int points = m_bezierControlPoints.size();
            
            if (points > 0) {
                // 显示从最后一个控制点到当前点的距离
                QPointF lastPoint = m_bezierControlPoints.back();
                double distance = sqrt(pow(scenePos.x() - lastPoint.x(), 2) + 
                                      pow(scenePos.y() - lastPoint.y(), 2));
                
                statusMsg = QString("Bezier曲线: %1个控制点 (距离上一点: %2像素) (左键添加点, 右键完成)")
                           .arg(points)
                           .arg(static_cast<int>(distance));
            } else {
                statusMsg = QString("Bezier曲线: %1个控制点 (左键添加点, 右键完成)").arg(points);
            }
        } else {
            QString typeName;
            switch (m_graphicType) {
                case Graphic::LINE: typeName = "直线"; break;
                case Graphic::RECTANGLE: typeName = "矩形"; break;
                case Graphic::ELLIPSE: typeName = "椭圆"; break;
                case Graphic::CIRCLE: typeName = "圆形"; break;
                default: typeName = "形状"; break;
            }
            statusMsg = QString("绘制%1 (%2, %3)").arg(typeName)
                        .arg(scenePos.x()).arg(scenePos.y());
        }
        
        mainWindow->statusBar()->showMessage(statusMsg);
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
            
            // 创建最终图形
            QGraphicsItem* finalItem = createFinalItem(drawArea);
            if (finalItem) {
                drawArea->scene()->addItem(finalItem);
            }
            
            // 重置状态
            m_isDrawing = false;
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
            m_isDrawing = false;
            
            // 移除临时预览项
            if (m_previewItem && drawArea->scene()) {
                drawArea->scene()->removeItem(m_previewItem);
                delete m_previewItem;
                m_previewItem = nullptr;
            }
            
            // 清除控制点标记
            clearControlPointMarkers(drawArea);
            
            // 清空控制点
            m_bezierControlPoints.clear();
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
    
    if (m_graphicType == Graphic::BEZIER) {
        // 为Bezier曲线使用特定的创建方法
        if (m_bezierControlPoints.size() >= 2) {
            item = drawArea->getGraphicFactory()->createCustomItem(m_graphicType, m_bezierControlPoints);
        }
    } else {
        // 其他图形使用标准创建方法
        std::vector<QPointF> points;
        points.push_back(m_startPoint);
        points.push_back(m_currentPoint);
        
        item = drawArea->getGraphicFactory()->createCustomItem(m_graphicType, points);
    }
    
    // 设置图形项的样式
    if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
        // 设置画笔和画刷
        QPen pen(m_lineColor, m_lineWidth);
        graphicItem->setPen(pen);
        
        if (m_fillMode) {
            graphicItem->setBrush(QBrush(m_fillColor));
        }
    }
    
    return item;
}

void DrawState::updatePreviewItem(DrawArea* drawArea)
{
    // 如果已有预览项，则移除它
    if (m_previewItem) {
        drawArea->scene()->removeItem(m_previewItem);
        delete m_previewItem;
        m_previewItem = nullptr;
    }
    
    // 创建新的预览项
    if (m_graphicType == Graphic::BEZIER) {
        // 为Bezier曲线创建预览
        if (m_bezierControlPoints.size() >= 1) {
            // 复制当前控制点列表，并添加当前鼠标位置作为临时点
            std::vector<QPointF> previewPoints = m_bezierControlPoints;
            previewPoints.push_back(m_currentPoint);
            
            m_previewItem = drawArea->getGraphicFactory()->createCustomItem(m_graphicType, previewPoints);
        }
    } else {
        // 其他图形使用标准创建方法
        std::vector<QPointF> points;
        points.push_back(m_startPoint);
        points.push_back(m_currentPoint);
        
        m_previewItem = drawArea->getGraphicFactory()->createCustomItem(m_graphicType, points);
    }
    
    // 设置预览项的样式
    if (m_previewItem) {
        // 添加到场景，确保在控制点标记下方
        m_previewItem->setZValue(0); // 低Z值确保在控制点标记下方
        drawArea->scene()->addItem(m_previewItem);
        
        // 设置半透明效果
        m_previewItem->setOpacity(0.6);
        
        // 设置画笔和画刷
        if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(m_previewItem)) {
            QPen pen(m_lineColor, m_lineWidth);
            pen.setStyle(Qt::DashLine); // 使用虚线作为预览
            graphicItem->setPen(pen);
            
            if (m_fillMode) {
                QBrush brush(m_fillColor);
                brush.setStyle(Qt::DiagCrossPattern); // 使用交叉线填充作为预览
                graphicItem->setBrush(brush);
            }
        }
        
        // 如果是贝塞尔曲线模式，确保控制点标记在最上方
        if (m_graphicType == Graphic::BEZIER) {
            // 将所有控制点标记移到最前面
            for (QGraphicsEllipseItem* marker : m_controlPointMarkers) {
                marker->setZValue(1000);
            }
        }
    }
}

void DrawState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    // 处理右键点击
    if (m_graphicType == Graphic::BEZIER && m_isDrawing) {
        qDebug() << "贝塞尔曲线: 接收到右键点击，完成贝塞尔曲线绘制，共" << m_bezierControlPoints.size() << "个控制点";
        
        // 创建最终的贝塞尔曲线
        if (m_bezierControlPoints.size() >= 2) {
            // 删除预览项
            if (m_previewItem) {
                drawArea->scene()->removeItem(m_previewItem);
                delete m_previewItem;
                m_previewItem = nullptr;
            }
            
            // 清除控制点标记
            clearControlPointMarkers(drawArea);
            
            // 创建最终的贝塞尔曲线
            QGraphicsItem* bezierItem = createFinalItem(drawArea);
            if (bezierItem) {
                drawArea->scene()->addItem(bezierItem);
                qDebug() << "贝塞尔曲线: 成功创建并添加到场景";
            }
            
            // 重置绘制状态
            m_isDrawing = false;
            m_bezierControlPoints.clear();
        } else {
            qDebug() << "贝塞尔曲线: 控制点不足，需要至少2个点";
        }
    }
}

void DrawState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    if (!m_isDrawing) {
        // 开始绘制操作
        m_isDrawing = true;
        m_startPoint = scenePos;
        m_currentPoint = scenePos;
        
        // 用于Bezier曲线：初始化控制点集合
        if (m_graphicType == Graphic::BEZIER) {
            m_bezierControlPoints.clear();
            m_bezierControlPoints.push_back(scenePos);
            qDebug() << "贝塞尔曲线: 开始绘制, 添加第一个控制点" << scenePos;
            
            // 绘制控制点标记
            drawControlPoints(drawArea);
        }
        
        // 创建预览图形
        updatePreviewItem(drawArea);
    } else if (m_graphicType == Graphic::BEZIER) {
        // 对于Bezier曲线，每次点击添加一个控制点
        m_bezierControlPoints.push_back(scenePos);
        qDebug() << "贝塞尔曲线: 添加控制点 #" << m_bezierControlPoints.size() << "位置:" << scenePos;
        
        // 更新预览
        if (m_previewItem) {
            drawArea->scene()->removeItem(m_previewItem);
            delete m_previewItem;
            m_previewItem = nullptr;
        }
        
        // 更新控制点标记
        drawControlPoints(drawArea);
        
        // 更新当前点
        m_currentPoint = scenePos;
        updatePreviewItem(drawArea);
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