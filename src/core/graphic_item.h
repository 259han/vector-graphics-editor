#ifndef GRAPHIC_ITEM_H
#define GRAPHIC_ITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <memory>
#include "graphic.h"
#include "draw_strategy.h"

// 自定义图形项基类，继承QGraphicsItem，同时使用原有的DrawStrategy
class GraphicItem : public QGraphicsItem {
public:
    GraphicItem();
    virtual ~GraphicItem() = default;
    
    // QGraphicsItem接口
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
    // 设置绘制策略
    void setDrawStrategy(std::shared_ptr<DrawStrategy> strategy);
    std::shared_ptr<DrawStrategy> getDrawStrategy() const { return m_drawStrategy; }
    
    // 几何变换方法（提供一致的API）
    virtual void moveBy(const QPointF& offset);
    virtual void rotateBy(double angle);
    virtual void scaleBy(double factor);
    virtual void mirror(bool horizontal);
    
    // 缩放相关方法
    virtual QPointF getScale() const;
    virtual void setScale(const QPointF& scale);
    // Override QGraphicsItem's setScale to maintain our custom scaling
    virtual void setScale(qreal scale);
    
    // 可移动性
    virtual bool isMovable() const { return m_isMovable; }
    virtual void setMovable(bool movable) { m_isMovable = movable; }
    
    // 类型相关
    virtual Graphic::GraphicType getGraphicType() const = 0;
    
    // 连接点管理
    virtual std::vector<QPointF> getConnectionPoints() const;
    virtual void addConnectionPoint(const QPointF& point);
    virtual void removeConnectionPoint(const QPointF& point);
    
    // 样式设置
    void setPen(const QPen& pen);
    void setBrush(const QBrush& brush);
    QPen getPen() const;
    QBrush getBrush() const;
    
    // 自定义用户数据
    QVariant itemData(int key) const;
    void setItemData(int key, const QVariant& value);
    
    // 控制点相关
    enum ControlHandle {
        None,
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
        Rotation
    };
    
    // 判断点击位置是否在控制点上
    ControlHandle handleAtPoint(const QPointF& point) const;
    
    // 辅助绘制方法
    void drawSelectionHandles(QPainter* painter);
    
    // 用于剪贴板功能的公共接口，获取绘图点集
    std::vector<QPointF> getClipboardPoints() const { return getDrawPoints(); }

    // 获取控制点大小
    static int getHandleSize() { return HANDLE_SIZE; }

protected:
    std::shared_ptr<DrawStrategy> m_drawStrategy;
    
    // 为DrawStrategy提供点集合
    virtual std::vector<QPointF> getDrawPoints() const = 0;
    
    QPen m_pen;
    QBrush m_brush;
    std::vector<QPointF> m_connectionPoints;
    double m_rotation = 0.0;
    QPointF m_scale = QPointF(1.0, 1.0);
    bool m_isMovable = true;
    
    // 用户数据
    QMap<int, QVariant> m_itemData;
    
    // 控制点大小
    static constexpr int HANDLE_SIZE = 12;  // 增大控制点大小以便更容易点击
    
    // 重写事件处理
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};

#endif // GRAPHIC_ITEM_H 