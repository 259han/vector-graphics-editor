#ifndef GRAPHIC_ITEM_H
#define GRAPHIC_ITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <memory>
#include <QPixmapCache>
#include <QPointF>
#include <QPen>
#include <QBrush>
#include <vector>
#include <QDataStream>
#include <QString>
#include <QPainterPath>

class DrawStrategy;

// 合并了Graphic和GraphicItem的功能
class GraphicItem : public QGraphicsItem {
public:
    // 从Graphic类迁移的图形类型枚举
    enum GraphicType {
        NONE = 0,       // 未指定
        LINE = 1,       // 直线
        RECTANGLE = 2,  // 矩形
        ELLIPSE = 3,    // 椭圆
        CIRCLE = 4,     // 圆形
        BEZIER = 5,     // Bezier曲线
        TRIANGLE = 6,   // 三角形
        FILL = 7,       // 填充
        // 后续添加更多图形...
        CONNECTION = 8, // 连接线
        CLIP = 9        // 裁剪操作
    };

    // 将图形类型转换为可读字符串
    static QString graphicTypeToString(GraphicType type) {
        switch (type) {
            case NONE: return "未指定";
            case LINE: return "直线";
            case RECTANGLE: return "矩形";
            case ELLIPSE: return "椭圆";
            case CIRCLE: return "圆形";
            case BEZIER: return "贝塞尔曲线";
            case TRIANGLE: return "三角形";
            case FILL: return "填充";
            case CONNECTION: return "连接线";
            case CLIP: return "裁剪";
            default: return "未知类型";
        }
    }

    GraphicItem();
    virtual ~GraphicItem() = default;
    
    // QGraphicsItem接口
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
    // 从Graphic继承的核心绘制方法
    virtual void draw(QPainter& painter) const;
    
    // 几何变换方法（提供一致的API）
    virtual void moveBy(const QPointF& offset);
    virtual void rotateBy(double angle);
    virtual void scaleBy(double factor);
    virtual void mirror(bool horizontal);
    
    // 从Graphic继承的几何变换方法别名，保持API兼容性
    virtual void move(const QPointF& offset) { moveBy(offset); }
    virtual void rotate(double angle) { rotateBy(angle); }
    virtual void scale(double factor) { scaleBy(factor); }
    
    // 缩放相关方法
    virtual QPointF getScale() const;
    virtual void setScale(const QPointF& scale);
    // Override QGraphicsItem's setScale to maintain our custom scaling
    virtual void setScale(qreal scale);
    
    // 可移动性
    virtual bool isMovable() const { return m_isMovable; }
    virtual void setMovable(bool movable) { m_isMovable = movable; }
    
    // 类型相关
    virtual GraphicType getGraphicType() const = 0;
    GraphicType getType() const { return getGraphicType(); } // 为保持兼容性

    // 从Graphic继承的方法
    virtual QPointF getCenter() const;
    virtual QRectF getBoundingBox() const { return boundingRect(); }

    // 从Graphic继承的碰撞检测方法
    virtual bool intersects(const QRectF& rect) const;
    virtual bool contains(const QPointF& point) const;
    
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
    
    // 设置绘制策略
    void setDrawStrategy(std::shared_ptr<DrawStrategy> strategy);
    std::shared_ptr<DrawStrategy> getDrawStrategy() const { return m_drawStrategy; }
    
    // 序列化和反序列化 (从Graphic接口)
    virtual void serialize(QDataStream& out) const;
    virtual void deserialize(QDataStream& in);
    
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

    // 缓存相关方法
    void enableCaching(bool enable);
    bool isCachingEnabled() const { return m_cachingEnabled; }
    void invalidateCache();
    
    // 裁剪相关方法
    virtual bool clip(const QPainterPath& clipPath);
    
    // 获取图形的路径表示（用于裁剪）
    virtual QPainterPath toPath() const;
    
    // 从点集合恢复图形（用于撤销裁剪）
    virtual void restoreFromPoints(const std::vector<QPointF>& points);

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
    
    // 裁剪相关成员
    bool m_useCustomPath = false;
    QPainterPath m_customClipPath;
    
    // 用户数据
    QMap<int, QVariant> m_itemData;
    
    // 控制点大小
    static constexpr int HANDLE_SIZE = 12;  // 增大控制点大小以便更容易点击
    
    // 重写事件处理
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    // 缓存相关属性
    bool m_cachingEnabled = false;
    QString m_cacheKey;
    bool m_cacheInvalid = true;
    QPixmap m_cachedPixmap;
    
    // 创建用于缓存的键
    QString createCacheKey() const;
    // 更新缓存
    void updateCache(QPainter *painter, const QStyleOptionGraphicsItem *option);
};

// 为保持兼容性，是GraphicItem的别名
typedef GraphicItem Graphic;

#endif // GRAPHIC_ITEM_H 