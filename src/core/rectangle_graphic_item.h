#ifndef RECTANGLE_GRAPHIC_ITEM_H
#define RECTANGLE_GRAPHIC_ITEM_H

#include "graphic_item.h"
#include <QPointF>
#include <QSizeF>
#include <QPainterPath>

// 矩形图形项
class RectangleGraphicItem : public GraphicItem {
public:
    RectangleGraphicItem(const QPointF& topLeft = QPointF(0, 0), const QSizeF& size = QSizeF(100, 60));
    ~RectangleGraphicItem() override = default;
    
    // 重写的QGraphicsItem方法
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    
    // 重写的形状方法，提供准确的碰撞检测
    QPainterPath shape() const override;
    
    // 实现GraphicItem抽象方法
    GraphicType getGraphicType() const override { return GraphicType::RECTANGLE; }
    
    // 获取绘制点
    std::vector<QPointF> getDrawPoints() const override;
    
    // 矩形特有的方法
    QPointF getTopLeft() const;
    void setTopLeft(const QPointF& topLeft);
    QSizeF getSize() const;
    void setSize(const QSizeF& size);
    
    // 重写碰撞检测方法
    bool contains(const QPointF& point) const override;
    
    // 重写缩放方法，以确保矩形特有的非均匀缩放处理
    void setScale(const QPointF& scale) override;
    void setScale(qreal scale) override;
    
    // 重写裁剪相关方法
    bool clip(const QPainterPath& clipPath) override;
    QPainterPath toPath() const override;
    void restoreFromPoints(const std::vector<QPointF>& points) override;
    
private:
    QPointF m_topLeft;  // 相对于中心点的偏移
    QSizeF m_size;      // 矩形基础尺寸
    
    // 自定义路径相关（用于非矩形裁剪结果）
    bool m_useCustomPath = false;          // 是否使用自定义路径
    QPainterPath m_customClipPath;         // 自定义裁剪路径
    QPointF m_customPathOffset;            // 自定义路径偏移量
};

#endif // RECTANGLE_GRAPHIC_ITEM_H 