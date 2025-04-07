#ifndef FILL_STATE_H
#define FILL_STATE_H

#include "editor_state.h"
#include <QColor>
#include <QPoint>
#include <QPointF>
#include <QStack>
#include <memory>
#include <QDebug>

class DrawArea;

class FillState : public EditorState {
public:
    FillState(const QColor& fillColor = Qt::red);
    virtual ~FillState() = default;
    
    // 处理鼠标和键盘事件
    void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) override;
    void paintEvent(DrawArea* drawArea, QPainter* painter) override;
    void keyPressEvent(DrawArea* drawArea, QKeyEvent* event) override;
    
    // 处理鼠标左右键点击
    void handleLeftMousePress(DrawArea* drawArea, QPointF scenePos) override;
    void handleRightMousePress(DrawArea* drawArea, QPointF scenePos) override;
    
    // 实现EditorState的纯虚函数
    bool isEditState() const override { return false; } // 填充状态不是编辑状态
    
    // 获取/设置填充颜色
    QColor getFillColor() const { return m_fillColor; }
    void setFillColor(const QColor& color) { 
        m_fillColor = color; 
        qDebug() << "FillState: 设置填充颜色为" << color << "RGBA(" 
                 << color.red() << "," << color.green() << "," 
                 << color.blue() << "," << color.alpha() << ")";
    }

private:
    // 填充封闭区域
    void fillRegion(DrawArea* drawArea, const QPointF& startPoint);
    
    // 使用扫描线算法填充图像区域，返回填充的像素数
    int fillImageRegion(QImage& image, const QPoint& seedPoint, 
                       const QColor& targetColor, const QColor& fillColor);
    
    // 属性
    QColor m_fillColor;   // 填充颜色
    QPointF m_lastPoint;  // 上次鼠标位置
    bool m_isPressed = false;
};

#endif // FILL_STATE_H
