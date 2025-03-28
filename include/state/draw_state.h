#ifndef DRAW_STATE_H
#define DRAW_STATE_H

#include "editor_state.h"
#include "../../include/core/graphic.h"
#include "../../include/core/draw_strategy.h"
#include <QColor>
#include <QPointF>
#include <vector>
#include <QStack>
#include <memory>

class DrawState : public EditorState {
public:
    DrawState(Graphic::GraphicType type);
    
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void setFillColor(const QColor& color);
    void setFillMode(bool enabled);
    void fillRegion(DrawArea* drawArea, QPointF startPoint, QColor targetColor, QColor fillColor);

private:
    Graphic::GraphicType m_graphicType;
    QPointF m_startPoint;      // 起始点
    QPointF m_currentPoint;    // 当前点
    bool m_isDrawing = false;
    QColor m_fillColor;        // 填充颜色
    bool m_fillMode = false;   // 填充模式开关
    QStack<QPoint> m_seedStack; // 用于种子填充的栈
    std::vector<QPointF> m_controlPoints; // 用于存储贝塞尔曲线的控制点
    std::shared_ptr<DrawStrategy> m_drawStrategy;
};

#endif // DRAW_STATE_H