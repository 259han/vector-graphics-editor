#include "draw_state.h"
#include "../ui/draw_area.h"
#include <QMouseEvent>
#include <cmath>
#include <QStack>
#include <QImage>

DrawState::DrawState(Graphic::GraphicType type) : m_graphicType(type) {}

void DrawState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (m_fillMode) {
        QPointF clickPoint = event->pos();
        // 直接使用 DrawArea 的 m_image
        QImage& canvasImage = drawArea->m_image; // 直接引用
        if (canvasImage.isNull()) {
            qDebug() << "Canvas image is null.";
            return;
        }

        QColor targetColor = canvasImage.pixelColor(clickPoint.toPoint());
        if (targetColor == m_fillColor) return;

        fillRegion(drawArea, clickPoint, targetColor, m_fillColor);
        drawArea->update();
    }  else {
        m_isDrawing = true;
        m_startPoint = event->pos();
        m_currentPoint = event->pos();
    }
}

void DrawState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (m_isDrawing) {
        m_currentPoint = event->pos();
        
        auto factory = drawArea->getGraphicFactory();
        std::unique_ptr<Graphic> graphic;
        std::vector<QPointF> points = {m_startPoint, m_currentPoint};
        
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
            // 其他图形类型（如 TRIANGLE）需要单独处理，此处略
            default:
                break;
        }
        
        if (graphic) {
            drawArea->getGraphicManager().addGraphic(std::move(graphic));
        }
        
        // 结束绘制
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