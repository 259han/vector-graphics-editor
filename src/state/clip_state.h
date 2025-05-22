#ifndef CLIP_STATE_H
#define CLIP_STATE_H

#include "editor_state.h"
#include "../core/graphic.h"
#include <QPointF>
#include <QColor>
#include <vector>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QPainterPath>
#include <memory>

class ClipState : public EditorState {
public:
    ClipState();
    ~ClipState() override = default;
    
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) override;
    
    // 状态切换通知
    void onEnterState(DrawArea* drawArea) override;
    void onExitState(DrawArea* drawArea) override;
    
    // 鼠标事件处理函数
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) override;
    
    // 状态类型和名称
    StateType getStateType() const override { return StateType::ClipState; }
    QString getStateName() const override { return "裁剪模式"; }
    
    // 设置裁剪区域模式
    enum ClipAreaMode {
        RectangleClip,  // 矩形裁剪
        FreehandClip    // 自由形状裁剪
    };
    
    void setClipAreaMode(ClipAreaMode mode);
    ClipAreaMode getClipAreaMode() const { return m_clipAreaMode; }
    
private:
    // 完成裁剪操作
    void finishClip(DrawArea* drawArea);
    
    // 取消裁剪操作
    void cancelClip(DrawArea* drawArea);
    
    // 更新裁剪预览
    void updateClipPreview(DrawArea* drawArea);
    
    // 清除临时图形项
    void clearTemporaryItems(DrawArea* drawArea);
    
    // 状态数据
    bool m_isClipping = false;
    QPointF m_startPoint;
    QPointF m_currentPoint;
    
    // 自由形状裁剪的点集合
    std::vector<QPointF> m_freehandPoints;
    
    // 裁剪区域模式
    ClipAreaMode m_clipAreaMode = RectangleClip;
    
    // 临时图形项
    QGraphicsRectItem* m_clipRectItem = nullptr;
    QGraphicsPathItem* m_clipPathItem = nullptr;
    
    // 选中的图形项
    QList<QGraphicsItem*> m_selectedItems;
    
    // 样式设置
    QColor m_clipAreaOutlineColor = QColor(0, 120, 215);
    QColor m_clipAreaFillColor = QColor(0, 120, 215, 40);
};

#endif // CLIP_STATE_H 