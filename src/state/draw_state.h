#ifndef DRAW_STATE_H
#define DRAW_STATE_H

#include "editor_state.h"
#include "../core/graphic.h"
#include "../core/draw_strategy.h"
#include "../core/graphics_item_factory.h"
#include <QColor>
#include <QPointF>
#include <vector>
#include <QStack>
#include <memory>
#include <QGraphicsItem>
#include <QGraphicsEllipseItem>

class DrawState : public EditorState {
public:
    explicit DrawState(Graphic::GraphicType type);
    ~DrawState() override = default;
    
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) override;
    
    // 鼠标事件处理函数
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos);
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos);
    
    // 设置视觉样式
    void setLineColor(const QColor& color);
    void setLineWidth(int width);
    void setFillColor(const QColor& color);
    
    // 设置填充模式
    void setFillMode(bool enabled);
    void resetFillMode();
    
    // 重写方法以实现接口
    bool isEditState() const override { return false; }
    
    // 调试方法 - 用于检查当前状态
    Graphic::GraphicType getCurrentGraphicType() const { return m_graphicType; }
    bool isDrawing() const { return m_isDrawing; }

    bool isFillMode() const { return m_fillMode; }
    QColor getFillColor() const { return m_fillColor; }

private:
    // 创建最终图形项
    QGraphicsItem* createFinalItem(DrawArea* drawArea);
    
    // 更新预览项
    void updatePreviewItem(DrawArea* drawArea);
    
    // 在场景上绘制控制点标记
    void drawControlPoints(DrawArea* drawArea);
    
    // 清除控制点标记
    void clearControlPointMarkers(DrawArea* drawArea);
    
    // 状态数据
    Graphic::GraphicType m_graphicType;
    QPointF m_startPoint;
    QPointF m_currentPoint;
    bool m_isDrawing = false;
    
    // Bezier曲线的控制点
    std::vector<QPointF> m_bezierControlPoints;
    
    // 控制点标记
    std::vector<QGraphicsEllipseItem*> m_controlPointMarkers;
    
    // 样式设置
    QColor m_lineColor = Qt::black;
    int m_lineWidth = 2;
    QColor m_fillColor = Qt::transparent;
    
    // 填充模式
    bool m_fillMode = false;
    
    // 绘制策略
    std::shared_ptr<DrawStrategy> m_drawStrategy;
    
    // 临时预览项
    QGraphicsItem* m_previewItem = nullptr;
};

#endif // DRAW_STATE_H