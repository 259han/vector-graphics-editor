#include "draw_state.h"
#include "../ui/draw_area.h"
#include <QMouseEvent>
#include <cmath>
#include <QStack>
#include <QImage>

DrawState::DrawState(Graphic::GraphicType type) : m_graphicType(type) {}

void DrawState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event) {
    // 1. 优先处理贝塞尔模式
    if (m_graphicType == Graphic::BEZIER) {
        if (event->button() == Qt::LeftButton) {
            // 左键：添加控制点
            m_controlPoints.push_back(event->pos());
            m_isDrawing = true;
            drawArea->update();
            return; // 关键：隔离事件处理
        } 
        else if (event->button() == Qt::RightButton) {
            // 右键：生成贝塞尔曲线（至少2个控制点）
            if (m_controlPoints.size() >= 2) {
                auto graphic = drawArea->getGraphicFactory()->createCustomGraphic(
                    Graphic::BEZIER, 
                    m_controlPoints // 传递实际控制点
                );
                if (graphic) {
                    drawArea->getGraphicManager().addGraphic(std::move(graphic));
                }
            }
            m_controlPoints.clear();
            m_isDrawing = false;
            drawArea->update();
            return; // 关键：隔离事件处理
        }
    }

    // 2. 处理填色模式
    if (m_fillMode && event->button() == Qt::LeftButton) {
        QPointF clickPoint = event->pos();
        QImage& canvasImage = drawArea->m_image;
        if (canvasImage.isNull()) return;

        QColor targetColor = canvasImage.pixelColor(clickPoint.toPoint());
        if (targetColor == m_fillColor) return;

        fillRegion(drawArea, clickPoint, targetColor, m_fillColor);
        drawArea->update();
        return;
    }

    // 3. 其他图形模式（直线、矩形等）
    if (event->button() == Qt::LeftButton) {
        if (!m_isDrawing) {
            m_controlPoints.clear();
            m_startPoint = QPointF();
            m_currentPoint = QPointF();
        }
        m_controlPoints.push_back(event->pos());
        if (m_controlPoints.size() == 1) {
            m_startPoint = event->pos();
        }
        m_currentPoint = event->pos();
        m_isDrawing = true;
        drawArea->update();
    }
}

void DrawState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) {
    // 关键：贝塞尔模式不处理释放事件
    if (m_isDrawing && m_graphicType != Graphic::BEZIER) { 
        m_currentPoint = event->pos();
        
        auto factory = drawArea->getGraphicFactory();
        std::unique_ptr<Graphic> graphic;
        std::vector<QPointF> points = {m_startPoint, m_currentPoint};
        
        // 根据类型创建图形
        switch (m_graphicType) {
            case Graphic::LINE:
                graphic = factory->createCustomGraphic(Graphic::LINE, points);
                break;
            case Graphic::RECTANGLE:
                graphic = factory->createCustomGraphic(Graphic::RECTANGLE, points);
                break;
            case Graphic::CIRCLE:
                graphic = factory->createCustomGraphic(Graphic::CIRCLE, points);
                break;
            case Graphic::ELLIPSE:
                graphic = factory->createCustomGraphic(Graphic::ELLIPSE, points);
                break;
            default:
                break;
        }
        
        if (graphic) {
            drawArea->getGraphicManager().addGraphic(std::move(graphic));
        }

        // 重置状态
        m_startPoint = QPointF();
        m_currentPoint = QPointF();
        m_controlPoints.clear();
        m_isDrawing = false;
        drawArea->update();
    }
}

void DrawState::fillRegion(DrawArea* drawArea, QPointF startPoint, QColor targetColor, QColor fillColor) {
    QImage* canvasImage = &(drawArea->m_image);
    if (canvasImage->isNull() || targetColor == fillColor) return;

    QPoint start = startPoint.toPoint();
    if (start.x() < 0 || start.y() < 0 || start.x() >= canvasImage->width() || start.y() >= canvasImage->height()) {
        return; // 坐标越界直接返回
    }

    // 使用扫描线算法优化填充
    QStack<QPoint> stack;
    stack.push(start);

    while (!stack.isEmpty()) {
        QPoint point = stack.pop();
        int x = point.x();
        int y = point.y();

        // 向左找到边界
        int left = x;
        while (left >= 0 && canvasImage->pixelColor(left, y) == targetColor) {
            left--;
        }
        left++;

        // 向右找到边界
        int right = x;
        while (right < canvasImage->width() && canvasImage->pixelColor(right, y) == targetColor) {
            right++;
        }
        right--;

        // 填充当前行
        bool spanAddedAbove = false;
        bool spanAddedBelow = false;
        for (int i = left; i <= right; i++) {
            canvasImage->setPixelColor(i, y, fillColor);

            // 检查上一行
            if (y > 0) {
                if (canvasImage->pixelColor(i, y - 1) == targetColor) {
                    if (!spanAddedAbove) {
                        stack.push(QPoint(i, y - 1));
                        spanAddedAbove = true;
                    }
                } else {
                    spanAddedAbove = false;
                }
            }

            // 检查下一行
            if (y < canvasImage->height() - 1) {
                if (canvasImage->pixelColor(i, y + 1) == targetColor) {
                    if (!spanAddedBelow) {
                        stack.push(QPoint(i, y + 1));
                        spanAddedBelow = true;
                    }
                } else {
                    spanAddedBelow = false;
                }
            }
        }
    }

    drawArea->update();
}

void DrawState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (m_isDrawing) {
        m_currentPoint = event->pos();
        drawArea->update(); // 更新绘图区域以显示预览
    }
}

void DrawState::paintEvent(DrawArea* drawArea, QPainter* painter) {
    if (m_isDrawing) {
        painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
        
        switch (m_graphicType) {
            case Graphic::LINE:
                painter->drawLine(m_startPoint, m_currentPoint);
                break;
            case Graphic::RECTANGLE: {
                QRectF rect(m_startPoint, m_currentPoint);
                painter->drawRect(rect);
                if (m_fillMode) {
                    QBrush brush(m_fillColor);
                    painter->fillRect(rect, brush);
                }
                break;
            }
            case Graphic::CIRCLE: {
                QPointF center = m_startPoint;
                double radius = std::sqrt(std::pow(m_currentPoint.x() - center.x(), 2) +
                                std::pow(m_currentPoint.y() - center.y(), 2));
                painter->drawEllipse(center, radius, radius);
                if (m_fillMode) {
                    painter->setBrush(QBrush(m_fillColor));
                    painter->drawEllipse(center, radius, radius);
                }
                break;
            }
            case Graphic::ELLIPSE: {
                QPointF center = m_startPoint;
                double width = std::abs(m_currentPoint.x() - center.x()) * 2;
                double height = std::abs(m_currentPoint.y() - center.y()) * 2;
                painter->drawEllipse(center, width / 2, height / 2);
                if (m_fillMode) {
                    painter->setBrush(QBrush(m_fillColor));
                    painter->drawEllipse(center, width / 2, height / 2);
                }
                break;
            }
            case Graphic::BEZIER: {
                // 1. 绘制控制点（蓝色圆点）
                painter->setPen(QPen(Qt::blue, 2));
                for (const auto& point : m_controlPoints) {
                    painter->drawEllipse(point, 3, 3);
                }

                // 2. 绘制控制点连线（灰色虚线）
                painter->setPen(QPen(Qt::gray, 1, Qt::DashLine));
                for (size_t i = 1; i < m_controlPoints.size(); ++i) {
                    painter->drawLine(m_controlPoints[i-1], m_controlPoints[i]);
                }

                // 3. 实时预览贝塞尔曲线（至少2个点）
                if (m_controlPoints.size() >= 2) {
                    BezierDrawStrategy bezierStrategy;
                    bezierStrategy.draw(*painter, m_controlPoints);
                }
                break;
            }
            default:
                break;
        }
    }
}

void DrawState::setFillColor(const QColor& color) {
    m_fillColor = color;
}

void DrawState::setFillMode(bool enabled) {
    m_fillMode = enabled;
}