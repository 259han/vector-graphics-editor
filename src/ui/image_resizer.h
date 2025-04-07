#ifndef IMAGE_RESIZER_H
#define IMAGE_RESIZER_H

#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QCursor>
#include <QDebug>
#include <QList>

/**
 * @brief 图像缩放和旋转控制器
 * 
 * 为图像提供交互式的调整大小和旋转功能。
 * 在选中图像时显示8个控制点和一个旋转控制点。
 */
class ImageResizer : public QGraphicsObject {
    Q_OBJECT
    
public:
    enum Handle {
        TopLeft = 0,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,
        Rotate = 8 // 旋转控制点
    };
    
    ImageResizer(QGraphicsItem* target);
    virtual ~ImageResizer();
    
    // QGraphicsItem接口
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
    bool sceneEvent(QEvent *event) override;
    
    // 更新控制点位置 - 公开方法，以便可以手动调用
    void updateHandles();
    
private:
    QGraphicsItem* m_target; // 目标项（要调整大小的图像）
    QList<QGraphicsRectItem*> m_handles; // 控制点
    QGraphicsLineItem* m_rotateLine; // 旋转控制线
    QGraphicsEllipseItem* m_rotateHandle; // 旋转控制点
    
    int m_currentHandle; // 当前活动控制点
    QPointF m_startPos; // 开始拖动位置
    QPointF m_originalCenter; // 原始中心点
    QSizeF m_originalSize; // 原始大小
    qreal m_originalRotation; // 原始旋转角度
    qreal m_startAngle; // 开始旋转角度
    
    // 创建调整大小的控制点
    void createHandles();
    
    // 鼠标光标形状
    Qt::CursorShape getCursorForHandle(int handle);
    
    // 调整大小
    void resize(const QPointF &pos);
    
    // 旋转
    void rotate(const QPointF &pos);
};

#endif // IMAGE_RESIZER_H 