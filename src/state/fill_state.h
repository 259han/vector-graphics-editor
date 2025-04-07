#ifndef FILL_STATE_H
#define FILL_STATE_H

#include "editor_state.h"
#include "../utils/logger.h"
#include <QColor>
#include <QPoint>
#include <QPointF>
#include <QStack>
#include <memory>
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <QList>
#include <QMainWindow>

class DrawArea;
class QGraphicsPixmapItem;

/**
 * @brief 颜色填充工具状态类
 * 
 * 实现颜色填充工具的功能，允许用户单击区域进行填充
 */
class FillState : public EditorState {
public:
    explicit FillState(const QColor& fillColor);
    ~FillState() override = default;
    
    // 处理鼠标和键盘事件
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) override;
    void wheelEvent(DrawArea* drawArea, QWheelEvent* event) override;
    
    // 状态切换通知
    void onEnterState(DrawArea* drawArea) override;
    void onExitState(DrawArea* drawArea) override;
    
    // 处理鼠标左右键点击
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleMiddleMousePress(DrawArea* drawArea, QPointF scenePos) override;
    
    // 填充操作
    void fillRegion(DrawArea* drawArea, const QPointF& startPoint);
    
    // 完成填充操作，创建并执行命令
    void finishOperation(DrawArea* drawArea);
    
    // 状态类型和名称
    StateType getStateType() const override { return StateType::FillState; }
    QString getStateName() const override { return "填充工具"; }
    
    // 获取/设置填充颜色
    QColor getFillColor() const { return m_fillColor; }
    void setFillColor(const QColor& color) { 
        m_fillColor = color; 
        Logger::debug(QString("FillState: 设置填充颜色为 %1 RGBA(%2,%3,%4,%5)")
                .arg(color.name(QColor::HexArgb))
                .arg(color.red()).arg(color.green())
                .arg(color.blue()).arg(color.alpha()));
    }

private:
    // 使用扫描线算法填充图像区域，返回填充的像素数
    int fillImageRegion(QImage& image, const QPoint& seedPoint, 
                       const QColor& targetColor, const QColor& fillColor);
    
    // 属性
    QColor m_fillColor;   // 填充颜色
    QPointF m_lastPoint;  // 上次鼠标位置
    QPointF m_currentPoint;
    bool m_isPressed = false;
};

#endif // FILL_STATE_H
