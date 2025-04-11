#include "graphic_item.h"
#include "../utils/logger.h"
#include <QStyleOption>
#include <QPainter>
#include <QGraphicsScene>
#include <QDebug>
#include <QTransform>
#include <cmath>

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

void GraphicItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    if (!m_drawStrategy) {
        // 如果没有绘制策略，使用默认绘制方法
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        
        // 默认简单绘制一个点
        painter->drawPoint(0, 0);
        return;
    }
    
    // 应用当前画笔和画刷设置到绘制策略
    m_drawStrategy->setColor(m_pen.color());
    m_drawStrategy->setLineWidth(m_pen.width());
    
    // 使用绘制策略绘制图形
    std::vector<QPointF> points = getDrawPoints();
    if (!points.empty()) {
        // 设置画笔和画刷
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        
        // 使用策略绘制图形
        m_drawStrategy->draw(painter, points);
    }
    
    // 如果被选中，绘制选中指示器
    if (isSelected()) {
        drawSelectionHandles(painter);
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

GraphicItem::ControlHandle GraphicItem::handleAtPoint(const QPointF& scenePoint) const
{
    // 将场景坐标转换为图形项的本地坐标
    QPointF itemPoint = mapFromScene(scenePoint);
    
    // 增大控制点的判定范围
    const qreal ENLARGED_HANDLE_SIZE = HANDLE_SIZE * 1.5;
    
    // 检查控制点时使用本地坐标
    QRectF rect = boundingRect();
    
    qDebug() << "GraphicItem::handleAtPoint - 场景坐标:" << scenePoint << "图形项坐标:" << itemPoint;
    
    // 检查是否在旋转控制点上
    QPointF rotateHandlePos(rect.center().x(), rect.top() - 20);
    if (QRectF(rotateHandlePos.x() - ENLARGED_HANDLE_SIZE/2, rotateHandlePos.y() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到旋转控制点";
        return Rotation;
    }
    
    // 检查左上控制点
    if (QRectF(rect.left() - ENLARGED_HANDLE_SIZE/2, rect.top() - ENLARGED_HANDLE_SIZE/2, 
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到左上控制点";
        return TopLeft;
    }
    
    // 检查上中控制点
    if (QRectF(rect.center().x() - ENLARGED_HANDLE_SIZE/2, rect.top() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到上中控制点";
        return TopCenter;
    }
    
    // 检查右上控制点
    if (QRectF(rect.right() - ENLARGED_HANDLE_SIZE/2, rect.top() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到右上控制点";
        return TopRight;
    }
    
    // 检查中左控制点
    if (QRectF(rect.left() - ENLARGED_HANDLE_SIZE/2, rect.center().y() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到中左控制点";
        return MiddleLeft;
    }
    
    // 检查中右控制点
    if (QRectF(rect.right() - ENLARGED_HANDLE_SIZE/2, rect.center().y() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到中右控制点";
        return MiddleRight;
    }
    
    // 检查左下控制点
    if (QRectF(rect.left() - ENLARGED_HANDLE_SIZE/2, rect.bottom() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到左下控制点";
        return BottomLeft;
    }
    
    // 检查下中控制点
    if (QRectF(rect.center().x() - ENLARGED_HANDLE_SIZE/2, rect.bottom() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到下中控制点";
        return BottomCenter;
    }
    
    // 检查右下控制点
    if (QRectF(rect.right() - ENLARGED_HANDLE_SIZE/2, rect.bottom() - ENLARGED_HANDLE_SIZE/2,
               ENLARGED_HANDLE_SIZE, ENLARGED_HANDLE_SIZE).contains(itemPoint)) {
        qDebug() << "GraphicItem::handleAtPoint - 检测到右下控制点";
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
    
    // 为了最大程度保持原始的外观，使用X和Y比例的平均值应用到QGraphicsItem
    // 注意：这是一个折中方案，因为QGraphicsItem本身不支持非均匀缩放
    qreal uniformScale = (scale.x() + scale.y()) / 2.0;
    
    // 确保缩放值合理
    if (uniformScale > 0.001) {
        QGraphicsItem::setScale(uniformScale);
    }
    
    update();
    
    Logger::debug(QString("GraphicItem::setScale - 设置缩放为 (%1, %2), 均匀缩放: %3")
                 .arg(scale.x(), 0, 'f', 3)
                 .arg(scale.y(), 0, 'f', 3)
                 .arg(uniformScale, 0, 'f', 3));
}

void GraphicItem::setScale(qreal scale)
{
    // 确保维持一致的X和Y比例
    m_scale = QPointF(scale, scale);
    
    // 调用基类实现
    if (scale > 0.001) {
        QGraphicsItem::setScale(scale);
    }
    
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
    
    // 同步更新DrawStrategy
    if (m_drawStrategy) {
        m_drawStrategy->setColor(pen.color());
        m_drawStrategy->setLineWidth(pen.width());
    }
    
    // 触发重绘
    update();
}

void GraphicItem::setBrush(const QBrush &brush)
{
    if (m_brush != brush) {
        // 保存旧值用于调试
        QBrush oldBrush = m_brush;
        
        // 设置新的画刷
        m_brush = brush;
        
        // 打印调试信息
        qDebug() << "GraphicItem::setBrush - 颜色从" << oldBrush.color().name() << "更改为" << brush.color().name();
        
        // 强制更新图形
        update();
    } else {
        qDebug() << "GraphicItem::setBrush - 忽略相同的画刷" << brush.color().name();
    }
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