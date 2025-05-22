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
#include <QScrollBar>
#include <QMainWindow>

class DrawState : public EditorState {
public:
    explicit DrawState(Graphic::GraphicType type);
    ~DrawState() override = default;
    
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) override;
    void wheelEvent(DrawArea* drawArea, QWheelEvent* event) override;
    
    // 状态切换通知
    void onEnterState(DrawArea* drawArea) override;
    void onExitState(DrawArea* drawArea) override;
    
    // 鼠标事件处理函数
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleMiddleMousePress(DrawArea* drawArea, QPointF scenePos) override;
    
    // 设置视觉样式
    void setLineColor(const QColor& color);
    void setLineWidth(int width);
    void setFillColor(const QColor& color);
    
    // 设置填充模式
    void setFillMode(bool enabled);
    void resetFillMode();
    
    // 状态类型和名称
    StateType getStateType() const override { return StateType::DrawState; }
    QString getStateName() const override { 
        switch (m_graphicType) {
            case Graphic::LINE: return "绘制直线";
            case Graphic::RECTANGLE: return "绘制矩形";
            case Graphic::ELLIPSE: return "绘制椭圆";
            case Graphic::CIRCLE: return "绘制圆形";
            case Graphic::BEZIER: return "绘制贝塞尔曲线";
            case Graphic::TRIANGLE: return "绘制三角形";
            case Graphic::FLOWCHART_PROCESS: return "绘制流程图处理框";
            case Graphic::FLOWCHART_DECISION: return "绘制流程图判断框";
            case Graphic::FLOWCHART_START_END: return "绘制流程图开始/结束框";
            case Graphic::FLOWCHART_IO: return "绘制流程图输入/输出框";
            case Graphic::FLOWCHART_CONNECTOR: return "绘制流程图连接器";
            default: return "绘制模式";
        }
    }
    
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
    
    // 工具方法
    QPointF getScenePos(DrawArea* drawArea, QMouseEvent* event);
    void updateStatusMessage(DrawArea* drawArea, const QString& message);
    void setCursor(DrawArea* drawArea, Qt::CursorShape shape);
    void resetCursor(DrawArea* drawArea);
    bool handleZoomAndPan(DrawArea* drawArea, QWheelEvent* event);
    
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