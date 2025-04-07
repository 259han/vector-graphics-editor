#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <QPointF>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <memory>
#include <vector>
#include <QDataStream>
#include <QString>

class DrawStrategy;

class Graphic {
public:
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
        CONNECTION = 8  // 连接线
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
            default: return "未知类型";
        }
    }

    Graphic();
    virtual ~Graphic() = default;

    // 核心绘制方法
    virtual void draw(QPainter& painter) const = 0;
    
    // 几何变换方法
    virtual void move(const QPointF& offset) = 0;
    virtual void rotate(double angle) = 0;
    virtual void scale(double factor) = 0;
    virtual void mirror(bool horizontal) = 0;  // 对称变换

    // 属性获取方法
    virtual QPointF getCenter() const = 0;
    virtual GraphicType getType() const = 0;
    
    // 包围盒方法，用于选择和碰撞检测
    virtual QRectF getBoundingBox() const = 0;

    // 设置绘制策略
    virtual void setDrawStrategy(std::shared_ptr<DrawStrategy> strategy) = 0;

    // 克隆方法，用于复制图形
    virtual std::unique_ptr<Graphic> clone() const = 0;

    // 序列化和反序列化
    virtual void serialize(QDataStream& out) const = 0;
    virtual void deserialize(QDataStream& in) = 0;

    // 样式设置
    void setPen(const QPen& pen) { m_pen = pen; }
    void setBrush(const QBrush& brush) { m_brush = brush; }
    QPen getPen() const { return m_pen; }
    QBrush getBrush() const { return m_brush; }

    // 选择状态
    void setSelected(bool selected) { m_selected = selected; }
    bool isSelected() const { return m_selected; }

    // 图层管理
    void setZValue(int z) { m_zValue = z; }
    int getZValue() const { return m_zValue; }

    // 连接点管理（用于流程图等）
    virtual std::vector<QPointF> getConnectionPoints() const = 0;
    virtual void addConnectionPoint(const QPointF& point) = 0;
    virtual void removeConnectionPoint(const QPointF& point) = 0;

    // 检查图形是否与选择区域相交
    virtual bool intersects(const QRectF& rect) const = 0;
    
    // 检查点是否在图形内
    virtual bool contains(const QPointF& point) const = 0;

protected:
    std::shared_ptr<DrawStrategy> m_drawStrategy;
    QPen m_pen;
    QBrush m_brush;
    bool m_selected = false;
    int m_zValue = 0;
};

#endif // GRAPHIC_H