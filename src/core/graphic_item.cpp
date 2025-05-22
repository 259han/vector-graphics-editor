#include "graphic_item.h"
#include "../utils/logger.h"
#include <QStyleOption>
#include <QPainter>
#include <QGraphicsScene>
#include <QDebug>
#include <QTransform>
#include <cmath>
#include <QCryptographicHash>
#include "draw_strategy.h"
#include <QApplication>

GraphicItem::GraphicItem()
{
    // 设置必要的标志，包括可选择和可移动
    setFlags(QGraphicsItem::ItemIsSelectable | 
             QGraphicsItem::ItemIsMovable | 
             QGraphicsItem::ItemSendsGeometryChanges);
    
    // 启用悬停事件
    setAcceptHoverEvents(true);
    
    // 设置默认画笔和画刷
    m_pen = QPen(Qt::black, 2); // 调整默认线宽与原Graphic一致
    m_brush = QBrush(Qt::transparent);
}

// 从Graphic类迁移的绘制方法
void GraphicItem::draw(QPainter& painter) const
{
    // 如果有绘制策略，使用策略进行绘制
    if (m_drawStrategy) {
        m_drawStrategy->setColor(m_pen.color());
        m_drawStrategy->setLineWidth(m_pen.width());
        
        std::vector<QPointF> points = getDrawPoints();
        if (!points.empty()) {
            painter.setPen(m_pen);
            painter.setBrush(m_brush);
            m_drawStrategy->draw(&painter, points);
        }
    } else {
        // 如果没有绘制策略，基本绘制
        painter.setPen(m_pen);
        painter.setBrush(m_brush);
        painter.drawPoint(0, 0);
    }
}

QRectF GraphicItem::boundingRect() const
{
    // 默认实现，子类应该重写此方法
    return QRectF();
}

void GraphicItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    // 设置抗锯齿和高质量渲染
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    // 优先使用自定义裁剪路径（用于非矩形形状）
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        // 保存当前的画笔和画刷
        QPen oldPen = painter->pen();
        QBrush oldBrush = painter->brush();
        
        // 设置画笔和画刷 - 确保只绘制轮廓
        painter->setPen(m_pen);
        painter->setBrush(Qt::NoBrush);
        
        // 明确禁用填充，仅绘制轮廓
        m_customClipPath.setFillRule(Qt::WindingFill);
        painter->drawPath(m_customClipPath);
        
        // 恢复原来的画笔和画刷
        painter->setPen(oldPen);
        painter->setBrush(oldBrush);
        
        // 如果选中，绘制控制柄
        if (isSelected()) {
            drawSelectionHandles(painter);
        }
        return;
    }
    
    // 默认绘制方法
    if (m_drawStrategy) {
        // 获取绘制点并使用绘制策略
        std::vector<QPointF> points = getDrawPoints();
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        m_drawStrategy->draw(painter, points);
    } else {
        // 基本绘制 - 使用图形项的标准设置
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        
        // 获取图形路径并绘制
        QPainterPath path = toPath();
        painter->drawPath(path);
    }
    
    // 绘制选择框（如果被选中）
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
    invalidateCache();
}

void GraphicItem::rotateBy(double angle)
{
    if (fabs(angle) > 0.01) {  // 使用一个小的阈值避免浮点误差
        m_rotation += angle;
        setRotation(m_rotation);
        invalidateCache();
    }
}

void GraphicItem::scaleBy(double factor)
{
    if (fabs(factor - 1.0) > 0.001) {  // 使用一个小的阈值避免浮点误差
        m_scale.rx() *= factor;
        m_scale.ry() *= factor;
        
        setScale(m_scale);
        invalidateCache();
    }
}

QPointF GraphicItem::getScale() const
{
    return m_scale;
}

void GraphicItem::setScale(const QPointF& scale)
{
    if (m_scale != scale) {
        m_scale = scale;
        invalidateCache();
        update();
    }
}

void GraphicItem::setScale(qreal scale)
{
    QPointF newScale(scale, scale);
    if (m_scale != newScale) {
        m_scale = newScale;
        invalidateCache();
        update();
    }
}

void GraphicItem::mirror(bool horizontal)
{
    if (horizontal) {
        m_scale.rx() *= -1;
    } else {
        m_scale.ry() *= -1;
    }
    setScale(m_scale);
    invalidateCache();
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
    if (m_pen != pen) {
        m_pen = pen;
        invalidateCache();
        update();
    }
}

void GraphicItem::setBrush(const QBrush &brush)
{
    if (m_brush != brush) {
        m_brush = brush;
        invalidateCache();
        update();
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
    // 处理位置变化
    else if (change == ItemPositionChange) {
        // 如果图形项被设置为不可移动，则阻止移动
        if (!m_isMovable) {
            return pos(); // 返回当前位置，不允许更改
        }
        
        // 返回新位置值，允许移动
        return value;
    }
    else if (change == ItemPositionHasChanged) {
        // 位置已经改变，更新相关状态
        invalidateCache();
        // 这里可以添加任何在位置变化后需要执行的逻辑
    }
    
    return QGraphicsItem::itemChange(change, value);
}

// 启用或禁用缓存
void GraphicItem::enableCaching(bool enable) {
    if (m_cachingEnabled != enable) {
        m_cachingEnabled = enable;
        m_cacheInvalid = true;
        
        if (!enable) {
            // 清除现有缓存
            m_cachedPixmap = QPixmap();
            m_cacheKey.clear();
        }
        
        update(); // 触发重绘
    }
}

// 使缓存无效
void GraphicItem::invalidateCache() {
    if (m_cachingEnabled) {
        m_cacheInvalid = true;
        update(); // 触发重绘以更新缓存
    }
}

// 悬停进入事件处理
void GraphicItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // 只有在可移动状态下才改变光标
    if (m_isMovable && (flags() & ItemIsMovable)) {
        // 设置鼠标光标为移动光标
        QApplication::setOverrideCursor(Qt::SizeAllCursor);
    }
    QGraphicsItem::hoverEnterEvent(event);
}

// 悬停离开事件处理
void GraphicItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    // 恢复默认光标
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    QGraphicsItem::hoverLeaveEvent(event);
}

// 创建用于缓存的键
QString GraphicItem::createCacheKey() const {
    // 使用图形项的属性创建唯一键
    QRectF rect = boundingRect();
    QString rectStr = QString("(%1,%2,%3,%4)").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
    
    QString key = QString("%1_%2_%3_%4_%5_%6_%7")
                     .arg(QString::number(reinterpret_cast<qulonglong>(this)))
                     .arg(rectStr)
                     .arg(m_pen.color().name(QColor::HexArgb))
                     .arg(m_pen.width())
                     .arg(m_brush.color().name(QColor::HexArgb))
                     .arg(m_rotation)
                     .arg(m_scale.x() * 100).arg(m_scale.y() * 100);
    
    // 创建点集的哈希
    QByteArray pointsData;
    QDataStream stream(&pointsData, QIODevice::WriteOnly);
    const auto points = getDrawPoints();
    for (const auto& point : points) {
        stream << point;
    }
    QByteArray hash = QCryptographicHash::hash(pointsData, QCryptographicHash::Md5).toHex();
    
    return key + "_" + QString::fromLatin1(hash);
}

// 更新缓存
void GraphicItem::updateCache(QPainter* painter, const QStyleOptionGraphicsItem* option) {
    // 对于基本图形，创建新缓存只在必要时进行
    if (m_cacheInvalid || m_cachedPixmap.isNull()) {
        // 创建缓存键
        m_cacheKey = createCacheKey();
        
        // 确定要缓存的大小
        QRect rect = boundingRect().toRect();
        if (rect.isEmpty()) {
            rect = QRect(0, 0, 1, 1); // 防止创建空的缓存图像
        }
        
        // 创建适合大小的pixmap
        m_cachedPixmap = QPixmap(rect.width(), rect.height());
        m_cachedPixmap.fill(Qt::transparent);
        
        // 在缓存上绘制图形内容
        QPainter cachePainter(&m_cachedPixmap);
        
        // 为平滑绘制设置抗锯齿
        cachePainter.setRenderHint(QPainter::Antialiasing, painter->renderHints() & QPainter::Antialiasing);
        cachePainter.setRenderHint(QPainter::SmoothPixmapTransform, painter->renderHints() & QPainter::SmoothPixmapTransform);
        
        // 应用必要的变换
        cachePainter.translate(-rect.topLeft());
        
        // 执行实际绘制（不包括选择处理）
        if (m_drawStrategy) {
            m_drawStrategy->setColor(m_pen.color());
            m_drawStrategy->setLineWidth(m_pen.width());
            
            std::vector<QPointF> points = getDrawPoints();
            if (!points.empty()) {
                cachePainter.setPen(m_pen);
                cachePainter.setBrush(m_brush);
                m_drawStrategy->draw(&cachePainter, points);
            }
        } else {
            // 没有策略时的默认绘制
            cachePainter.setPen(m_pen);
            cachePainter.setBrush(m_brush);
            cachePainter.drawPoint(0, 0);
        }
        
        // 标记缓存为有效
        m_cacheInvalid = false;
    }
    
    // 绘制缓存的pixmap
    painter->drawPixmap(boundingRect().toRect(), m_cachedPixmap);
    
    // 如果被选中，仍然需要绘制选中指示器（不缓存）
    if (isSelected()) {
        drawSelectionHandles(painter);
    }
}

// 获取中心点
QPointF GraphicItem::getCenter() const
{
    return boundingRect().center();
}

// 判断图形是否与矩形相交
bool GraphicItem::intersects(const QRectF& rect) const
{
    return boundingRect().intersects(rect);
}

// 判断点是否在图形内
bool GraphicItem::contains(const QPointF& point) const
{
    return QGraphicsItem::contains(mapFromScene(point));
}

// 序列化方法
void GraphicItem::serialize(QDataStream& out) const
{
    // 保存图形类型
    out << static_cast<int>(getGraphicType());
    
    // 保存位置
    out << pos();
    
    // 保存画笔和画刷
    out << m_pen;
    out << m_brush;
    
    // 保存旋转角度和缩放
    out << m_rotation;
    out << m_scale;
    
    // 保存绘图点
    std::vector<QPointF> points = getDrawPoints();
    out << static_cast<qint32>(points.size());
    for (const auto& point : points) {
        out << point;
    }
    
    // 保存连接点
    out << static_cast<qint32>(m_connectionPoints.size());
    for (const auto& point : m_connectionPoints) {
        out << point;
    }
}

// 反序列化方法
void GraphicItem::deserialize(QDataStream& in)
{
    // 图形类型已经确定，跳过
    int type;
    in >> type;
    
    // 读取位置
    QPointF position;
    in >> position;
    setPos(position);
    
    // 读取画笔和画刷
    in >> m_pen;
    in >> m_brush;
    
    // 读取旋转角度和缩放
    in >> m_rotation;
    in >> m_scale;
    setRotation(m_rotation);
    setScale(m_scale);
    
    // 读取连接点
    qint32 connectionPointCount;
    in >> connectionPointCount;
    m_connectionPoints.clear();
    for (qint32 i = 0; i < connectionPointCount; ++i) {
        QPointF point;
        in >> point;
        m_connectionPoints.push_back(point);
    }
}

bool GraphicItem::clip(const QPainterPath& clipPath)
{
    // 基类默认实现，子类应该重写以提供实际裁剪逻辑
    Logger::warning("GraphicItem::clip: 基类默认实现被调用，未执行实际裁剪");
    return false;
}

QPainterPath GraphicItem::toPath() const
{
    // 基类默认实现，子类应该重写以提供正确的路径表示
    Logger::warning("GraphicItem::toPath: 基类默认实现被调用，返回空路径");
    return QPainterPath();
}

void GraphicItem::restoreFromPoints(const std::vector<QPointF>& points)
{
    // 基类默认实现，子类应该重写以支持从点集恢复图形
    Logger::warning("GraphicItem::restoreFromPoints: 基类默认实现被调用，未执行实际恢复");
} 