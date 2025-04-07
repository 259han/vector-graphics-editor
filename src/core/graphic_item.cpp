#include "graphic_item.h"

GraphicItem::GraphicItem()
{
    // 设置必要的标志，包括可选择和可移动
    setFlags(QGraphicsItem::ItemIsSelectable | 
             QGraphicsItem::ItemIsMovable | 
             QGraphicsItem::ItemSendsGeometryChanges);
    
    // 启用悬停事件
    setAcceptHoverEvents(true);
    
    // 设置默认画笔和画刷
    m_pen = QPen(Qt::black, 1);
    m_brush = QBrush(Qt::transparent);
}

QRectF GraphicItem::boundingRect() const
{
    // 默认实现，子类应该重写此方法
    return QRectF();
}

void GraphicItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    // 使用DrawStrategy进行绘制
    if (m_drawStrategy) {
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        
        // 获取绘制点集合
        std::vector<QPointF> points = getDrawPoints();
        
        // 使用策略模式绘制
        m_drawStrategy->draw(*painter, points);
        
        // 如果选中，绘制选择框和控制点
        if (isSelected()) {
            drawSelectionHandles(painter);
        }
    }
}

void GraphicItem::drawSelectionHandles(QPainter* painter)
{
    // 绘制选择框
    QPen selectionPen(Qt::blue, 1, Qt::DashLine);
    painter->setPen(selectionPen);
    painter->setBrush(Qt::transparent);
    painter->drawRect(boundingRect());
    
    // 保存原来的笔和画刷
    QPen origPen = painter->pen();
    QBrush origBrush = painter->brush();
    
    // 设置控制点样式
    painter->setPen(QPen(Qt::blue, 1));
    painter->setBrush(QBrush(Qt::white));
    
    // 获取边界矩形
    QRectF rect = boundingRect();
    
    // 绘制8个控制点
    // 左上
    painter->drawRect(QRectF(rect.left() - HANDLE_SIZE/2, rect.top() - HANDLE_SIZE/2, 
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 上中
    painter->drawRect(QRectF(rect.center().x() - HANDLE_SIZE/2, rect.top() - HANDLE_SIZE/2,
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 右上
    painter->drawRect(QRectF(rect.right() - HANDLE_SIZE/2, rect.top() - HANDLE_SIZE/2,
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 中左
    painter->drawRect(QRectF(rect.left() - HANDLE_SIZE/2, rect.center().y() - HANDLE_SIZE/2,
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 中右
    painter->drawRect(QRectF(rect.right() - HANDLE_SIZE/2, rect.center().y() - HANDLE_SIZE/2,
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 左下
    painter->drawRect(QRectF(rect.left() - HANDLE_SIZE/2, rect.bottom() - HANDLE_SIZE/2,
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 下中
    painter->drawRect(QRectF(rect.center().x() - HANDLE_SIZE/2, rect.bottom() - HANDLE_SIZE/2,
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 右下
    painter->drawRect(QRectF(rect.right() - HANDLE_SIZE/2, rect.bottom() - HANDLE_SIZE/2,
                     HANDLE_SIZE, HANDLE_SIZE));
    
    // 绘制旋转控制点
    painter->setPen(QPen(Qt::red, 1));
    painter->setBrush(QBrush(Qt::white));
    painter->drawEllipse(QPointF(rect.center().x(), rect.top() - 20), HANDLE_SIZE/2, HANDLE_SIZE/2);
    painter->drawLine(rect.center().x(), rect.top(), rect.center().x(), rect.top() - 20);
    
    // 恢复原来的笔和画刷
    painter->setPen(origPen);
    painter->setBrush(origBrush);
}

GraphicItem::ControlHandle GraphicItem::handleAtPoint(const QPointF& point) const
{
    QRectF rect = boundingRect();
    
    // 检查是否在旋转控制点上
    QPointF rotateHandlePos(rect.center().x(), rect.top() - 20);
    if (QRectF(rotateHandlePos.x() - HANDLE_SIZE/2, rotateHandlePos.y() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return Rotation;
    }
    
    // 检查左上控制点
    if (QRectF(rect.left() - HANDLE_SIZE/2, rect.top() - HANDLE_SIZE/2, 
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return TopLeft;
    }
    
    // 检查上中控制点
    if (QRectF(rect.center().x() - HANDLE_SIZE/2, rect.top() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return TopCenter;
    }
    
    // 检查右上控制点
    if (QRectF(rect.right() - HANDLE_SIZE/2, rect.top() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return TopRight;
    }
    
    // 检查中左控制点
    if (QRectF(rect.left() - HANDLE_SIZE/2, rect.center().y() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return MiddleLeft;
    }
    
    // 检查中右控制点
    if (QRectF(rect.right() - HANDLE_SIZE/2, rect.center().y() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return MiddleRight;
    }
    
    // 检查左下控制点
    if (QRectF(rect.left() - HANDLE_SIZE/2, rect.bottom() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return BottomLeft;
    }
    
    // 检查下中控制点
    if (QRectF(rect.center().x() - HANDLE_SIZE/2, rect.bottom() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return BottomCenter;
    }
    
    // 检查右下控制点
    if (QRectF(rect.right() - HANDLE_SIZE/2, rect.bottom() - HANDLE_SIZE/2,
               HANDLE_SIZE, HANDLE_SIZE).contains(point)) {
        return BottomRight;
    }
    
    return None;
}

void GraphicItem::setDrawStrategy(std::shared_ptr<DrawStrategy> strategy)
{
    m_drawStrategy = strategy;
    update();
}

void GraphicItem::moveBy(const QPointF &offset)
{
    QGraphicsItem::moveBy(offset.x(), offset.y());
}

void GraphicItem::rotateBy(double angle)
{
    m_rotation += angle;
    setRotation(rotation() + angle);
}

void GraphicItem::scaleBy(double factor)
{
    QPointF newScale = m_scale * factor;
    setScale(newScale);
}

QPointF GraphicItem::getScale() const
{
    return m_scale;
}

void GraphicItem::setScale(const QPointF& scale)
{
    m_scale = scale;
    
    // Apply the scale to the QGraphicsItem using the average of x and y
    qreal uniformScale = (scale.x() + scale.y()) / 2.0;
    QGraphicsItem::setScale(uniformScale);
    
    update();
}

void GraphicItem::setScale(qreal scale)
{
    // Update our internal scale to maintain consistent x and y scales
    m_scale = QPointF(scale, scale);
    
    // Call the base class implementation
    QGraphicsItem::setScale(scale);
    
    update();
}

void GraphicItem::mirror(bool horizontal)
{
    // 实现水平或垂直镜像
    QTransform transform;
    if (horizontal) {
        transform.scale(-1, 1);
    } else {
        transform.scale(1, -1);
    }
    setTransform(transform, true);
}

std::vector<QPointF> GraphicItem::getConnectionPoints() const
{
    return m_connectionPoints;
}

void GraphicItem::addConnectionPoint(const QPointF &point)
{
    m_connectionPoints.push_back(point);
}

void GraphicItem::removeConnectionPoint(const QPointF &point)
{
    auto it = std::find_if(m_connectionPoints.begin(), m_connectionPoints.end(),
                          [&point](const QPointF& p) {
                              const double EPSILON = 5.0;
                              return (QPointF(p - point).manhattanLength() < EPSILON);
                          });
    
    if (it != m_connectionPoints.end()) {
        m_connectionPoints.erase(it);
    }
}

void GraphicItem::setPen(const QPen &pen)
{
    m_pen = pen;
    update();
}

void GraphicItem::setBrush(const QBrush &brush)
{
    m_brush = brush;
    update();
}

QPen GraphicItem::getPen() const
{
    return m_pen;
}

QBrush GraphicItem::getBrush() const
{
    return m_brush;
}

QVariant GraphicItem::itemData(int key) const
{
    return m_itemData.value(key);
}

void GraphicItem::setItemData(int key, const QVariant& value)
{
    m_itemData[key] = value;
}

QVariant GraphicItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // 处理选择状态变化
    if (change == ItemSelectedHasChanged) {
        update(); // 更新绘制以显示/隐藏控制点
    }
    
    return QGraphicsItem::itemChange(change, value);
} 