#include "rectangle_graphic_item.h"
#include <QPainter>
#include "../utils/logger.h"
#include "draw_strategy.h"
#include "../utils/clip_algorithms.h"

RectangleGraphicItem::RectangleGraphicItem(const QPointF& topLeft, const QSizeF& size)
{
    // 设置默认画笔和画刷
    m_pen.setColor(Qt::black);
    m_pen.setWidth(2);
    m_brush = Qt::NoBrush;
    
    // 设置绘制策略
    m_drawStrategy = std::make_shared<RectangleDrawStrategy>();
    // 确保DrawStrategy使用正确的画笔设置
    m_drawStrategy->setColor(m_pen.color());
    m_drawStrategy->setLineWidth(m_pen.width());
    
    // 确保矩形至少有最小尺寸
    QSizeF validSize(std::max(1.0, size.width()), std::max(1.0, size.height()));
    m_size = validSize;
    
    // 设置位置
    setPos(topLeft + QPointF(validSize.width()/2, validSize.height()/2)); // 设置中心点为位置
    
    // 相对于坐标系原点的偏移
    m_topLeft = QPointF(-validSize.width()/2, -validSize.height()/2);
}

QRectF RectangleGraphicItem::boundingRect() const
{
    // 如果使用自定义路径，使用自定义路径的边界
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        // 获取自定义路径的边界矩形
        QRectF pathBounds = m_customClipPath.boundingRect();
        
        // 增加一些边距确保能正确显示边框
        qreal extra = m_pen.width() + 5.0;
        return pathBounds.adjusted(-extra, -extra, extra, extra);
    }
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 基于缩放后的尺寸计算左上角相对位置
    QPointF scaledTopLeft = QPointF(-scaledWidth/2, -scaledHeight/2);
    
    // 边界矩形，相对于图形项坐标系的原点
    // 增加一些边距确保能正确显示边框
    qreal extra = m_pen.width() + 5.0;
    return QRectF(
        scaledTopLeft.x() - extra,
        scaledTopLeft.y() - extra,
        scaledWidth + extra * 2,
        scaledHeight + extra * 2
    );
}

// 添加shape方法实现准确的矩形碰撞检测
QPainterPath RectangleGraphicItem::shape() const
{
    // 如果使用自定义路径，返回自定义路径的形状
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        // 使用自定义裁剪路径作为形状
        QPainterPath customShape = m_customClipPath;
        
        // 考虑画笔宽度的影响，使用strokePath扩展路径
        QPainterPathStroker stroker;
        stroker.setWidth(m_pen.width());
        return customShape.united(stroker.createStroke(customShape));
    }
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 基于缩放后的尺寸计算左上角相对位置
    QPointF scaledTopLeft = QPointF(-scaledWidth/2, -scaledHeight/2);
    
    // 创建矩形路径
    QPainterPath path;
    path.addRect(QRectF(
        scaledTopLeft.x(),
        scaledTopLeft.y(),
        scaledWidth,
        scaledHeight
    ));
    
    // 考虑画笔宽度的影响，使用strokePath扩展路径
    QPainterPathStroker stroker;
    stroker.setWidth(m_pen.width());
    return path.united(stroker.createStroke(path));
}

std::vector<QPointF> RectangleGraphicItem::getDrawPoints() const
{
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 基于缩放后的尺寸计算左上角相对位置
    QPointF scaledTopLeft = QPointF(-scaledWidth/2, -scaledHeight/2);
    
    // 提供给DrawStrategy的点集合
    // 对于矩形，需要左上角和右下角点
    return {
        scaledTopLeft,
        scaledTopLeft + QPointF(scaledWidth, scaledHeight)
    };
}

QPointF RectangleGraphicItem::getTopLeft() const
{
    // 返回全局坐标系中的左上角点
    return pos() + m_topLeft;
}

void RectangleGraphicItem::setTopLeft(const QPointF& topLeft)
{
    // 更新位置，保持大小不变
    setPos(topLeft + QPointF(m_size.width()/2, m_size.height()/2));
    update();
}

QSizeF RectangleGraphicItem::getSize() const
{
    return m_size;
}

void RectangleGraphicItem::setSize(const QSizeF& size)
{
    // 确保矩形至少有最小尺寸
    QSizeF validSize(std::max(1.0, size.width()), std::max(1.0, size.height()));
    
    // 更新大小，保持左上角位置不变
    QPointF oldTopLeft = getTopLeft();
    m_size = validSize;
    
    // 重新计算中心点和左上角相对坐标
    setPos(oldTopLeft + QPointF(validSize.width()/2, validSize.height()/2));
    m_topLeft = QPointF(-validSize.width()/2, -validSize.height()/2);
    
    update();
}

void RectangleGraphicItem::setScale(const QPointF& scale)
{
    // 设置新的基础比例
    m_scale = scale;
    
    // 直接应用scale进行绘制，而不是改变m_size
    // 这样可以避免累积放大效应
    
    // 绘制时的尺寸会根据getDrawPoints()计算，因此不需要修改m_size
    
    // 设置QGraphicsItem的基础缩放为1.0，让我们自行处理缩放
    QGraphicsItem::setScale(1.0);
    
    Logger::debug(QString("RectangleGraphicItem::setScale - 设置为(%1, %2), 基础尺寸: %3x%4")
                 .arg(scale.x(), 0, 'f', 3)
                 .arg(scale.y(), 0, 'f', 3)
                 .arg(m_size.width())
                 .arg(m_size.height()));
    
    // 更新图形
    update();
}

void RectangleGraphicItem::setScale(qreal scale)
{
    // 使用相同的x和y缩放
    setScale(QPointF(scale, scale));
}

// 实现contains方法，判断点是否在矩形内
bool RectangleGraphicItem::contains(const QPointF& point) const
{
    // 如果使用自定义路径，检查点是否在自定义路径内
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        // 将点从场景坐标转换为图形项的本地坐标
        QPointF localPoint = mapFromScene(point);
        
        // 检查点是否在路径内部或边缘上
        // 考虑笔宽的影响
        QPainterPathStroker stroker;
        stroker.setWidth(m_pen.width() + 2.0); // 增加额外的容差
        QPainterPath expandedPath = m_customClipPath.united(stroker.createStroke(m_customClipPath));
        return expandedPath.contains(localPoint);
    }
    
    // 对于普通矩形，使用原来的检测方法
    // 将点从场景坐标转换为图形项的本地坐标
    QPointF localPoint = mapFromScene(point);
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 计算矩形边界（相对于图形项中心）
    double halfWidth = scaledWidth / 2.0;
    double halfHeight = scaledHeight / 2.0;
    
    // 扩展边界以包含画笔宽度
    double penWidth = m_pen.width();
    double tolerance = penWidth + 2.0; // 增加额外的容差
    
    // 检查点是否在扩展的矩形内
    return (localPoint.x() >= -halfWidth - tolerance && 
            localPoint.x() <= halfWidth + tolerance &&
            localPoint.y() >= -halfHeight - tolerance && 
            localPoint.y() <= halfHeight + tolerance);
}

bool RectangleGraphicItem::clip(const QPainterPath& clipPath)
{
    Logger::debug("RectangleGraphicItem::clip: 开始执行矩形裁剪(使用通用裁剪算法)");

    // 获取矩形的当前边界
    QRectF bounds = boundingRect();
    bounds.translate(pos());  // 转换为场景坐标
    Logger::debug(QString("RectangleGraphicItem::clip: 原始形状边界: (%1,%2,%3,%4)")
        .arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height()));

    // 将裁剪路径转换为点集
    std::vector<QPointF> clipPoints = ClipAlgorithms::pathToPoints(clipPath, 0.5);
    Logger::debug(QString("pathToPoints: 通过toFillPolygon提取了 %1 个点").arg(clipPoints.size()));

    // 是否是自由形状裁剪（需要用通用算法）
    if (clipPoints.size() > 4) {
        Logger::debug("RectangleGraphicItem::clip: 使用通用裁剪算法(自由形状裁剪)");

        // 获取矩形的路径
        QPainterPath path = toPath();
        // 转换到场景坐标
        QTransform toScene;
        toScene.translate(pos().x(), pos().y());
        path = toScene.map(path);

        // 使用裁剪算法计算路径交集
        QPainterPath resultPath = ClipAlgorithms::clipPath(path, clipPath);
        Logger::debug(QString("RectangleGraphicItem::clip: 裁剪结果元素数: %1, 点数: %2")
            .arg(resultPath.elementCount())
            .arg(ClipAlgorithms::pathToPoints(resultPath, 0.5).size()));

        // 判断裁剪结果是否是矩形
        bool isRectangular = ClipAlgorithms::isPathRectangular(resultPath);
        
        if (isRectangular) {
            // 如果裁剪结果是矩形，直接调整矩形大小和位置
            QRectF resultRect = resultPath.boundingRect();
            setPos(resultRect.center());
            
            // 将尺寸设置为新矩形的尺寸
            QSizeF newSize(resultRect.width(), resultRect.height());
            m_size = newSize;
            m_topLeft = QPointF(-newSize.width()/2, -newSize.height()/2);
            
            // 确保不使用自定义路径
            m_useCustomPath = false;
            
            // 更新缓存和图形显示
            invalidateCache();
            update();
            
            Logger::info(QString("RectangleGraphicItem::clip: 裁剪结果是矩形，尺寸: %1x%2")
                        .arg(newSize.width())
                        .arg(newSize.height()));
        } else {
            // 非矩形裁剪结果，转换为自定义形状
            std::vector<QPointF> resultPoints = ClipAlgorithms::pathToPoints(resultPath, 0.5);
            Logger::debug(QString("RectangleGraphicItem::clip: 裁剪结果不是矩形，点数: %1")
                         .arg(resultPoints.size()));
            
            if (resultPoints.size() < 3) {
                Logger::warning("RectangleGraphicItem::clip: 裁剪结果点数不足，无法创建有效形状");
                return false;
            }
            
            // 计算结果边界和中心点
            QRectF resultBounds = resultPath.boundingRect();
            Logger::debug(QString("RectangleGraphicItem::clip: 裁剪结果边界: (%1,%2,%3,%4)")
                         .arg(resultBounds.x()).arg(resultBounds.y())
                         .arg(resultBounds.width()).arg(resultBounds.height()));
            
            // 对裁剪结果路径进行预处理，确保质量
            // 使用更小的flatness值获取更精细的点集
            std::vector<QPointF> detailedPoints = ClipAlgorithms::pathToPoints(resultPath, 0.1);
            
            Logger::debug(QString("RectangleGraphicItem::clip: 优化后的裁剪结果点数: %1")
                         .arg(detailedPoints.size()));
            
            // 检查裁剪结果是否有效
            if (detailedPoints.size() < 3) {
                Logger::warning("RectangleGraphicItem::clip: 裁剪结果点数不足，无法创建有效路径");
                return false;
            }
            
            // 计算结果的中心点和边界
            QRectF newBounds = resultPath.boundingRect();
            QPointF newCenter = newBounds.center();
            
            // 设置图形项位置为裁剪结果的中心
            setPos(newCenter);
            
            // 将裁剪结果转换为相对于新中心点的坐标
            QTransform transform;
            transform.translate(-newCenter.x(), -newCenter.y());
            m_customClipPath = transform.map(resultPath);
            
            // 最后再次检查自定义路径是否有效
            if (m_customClipPath.isEmpty()) {
                Logger::warning("RectangleGraphicItem::clip: 转换后的自定义路径为空，保持原图形不变");
                return false;
            }
            
            // 保留WindingFill作为计算规则，但我们的绘制会忽略填充
            m_customClipPath.setFillRule(Qt::WindingFill);
            
            // 启用自定义路径绘制模式
            m_useCustomPath = true;
            
            // 保存尺寸信息（主要用于边界框计算）
            m_size = newBounds.size();
            m_topLeft = QPointF(-m_size.width()/2, -m_size.height()/2);
            
            // 如果点太多，尝试简化点集以提高性能
            if (detailedPoints.size() > 500) {
                Logger::debug("RectangleGraphicItem::clip: 尝试简化过多的点");
                // 使用更大的flatness值重新生成路径点，减少点数
                std::vector<QPointF> simplifiedPoints = ClipAlgorithms::pathToPoints(m_customClipPath, 0.5);
                if (simplifiedPoints.size() >= 3 && simplifiedPoints.size() < detailedPoints.size()) {
                    Logger::debug(QString("RectangleGraphicItem::clip: 成功简化点数从 %1 到 %2")
                                 .arg(detailedPoints.size())
                                 .arg(simplifiedPoints.size()));
                    
                    // 重新创建简化后的路径
                    m_customClipPath = ClipAlgorithms::pointsToPath(simplifiedPoints);
                    m_customClipPath.setFillRule(Qt::WindingFill);
                    detailedPoints = simplifiedPoints;
                }
            }
            
            // 更新缓存和图形显示
            invalidateCache();
            update();
            
            Logger::info(QString("RectangleGraphicItem::clip: 裁剪完成，转换为自定义形状，点数: %1")
                        .arg(detailedPoints.size()));
        }
    }
    
    return true;
}

QPainterPath RectangleGraphicItem::toPath() const
{
    // 如果使用自定义路径，直接返回自定义路径
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        return m_customClipPath;
    }
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_size.width() * m_scale.x();
    double scaledHeight = m_size.height() * m_scale.y();
    
    // 基于缩放后的尺寸计算左上角相对位置
    QPointF scaledTopLeft = QPointF(-scaledWidth/2, -scaledHeight/2);
    
    // 创建矩形路径（相对于图形项坐标系）
    QPainterPath path;
    path.addRect(QRectF(
        scaledTopLeft.x(),
        scaledTopLeft.y(),
        scaledWidth,
        scaledHeight
    ));
    
    // 应用变换（旋转）
    if (m_rotation != 0.0) {
        QTransform transform;
        transform.rotate(m_rotation);
        path = transform.map(path);
    }
    
    return path;
}

void RectangleGraphicItem::restoreFromPoints(const std::vector<QPointF>& points)
{
    // 检查点集合大小
    if (points.size() < 2) {
        Logger::warning("RectangleGraphicItem::restoreFromPoints: 点集合不足，无法恢复");
        return;
    }
    
    // 如果点数大于4，可能是非矩形形状，使用自定义路径
    if (points.size() > 4) {
        Logger::debug(QString("RectangleGraphicItem::restoreFromPoints: 检测到非矩形形状，点数: %1")
                    .arg(points.size()));
        
        // 创建路径
        QPainterPath path = ClipAlgorithms::pointsToPath(points);
        
        // 设置边界矩形作为边界框
        QRectF bounds = path.boundingRect();
        
        // 将点集合转换为相对于新中心点的坐标
        QPointF center = bounds.center();
        
        // 设置自定义路径
        m_useCustomPath = true;
        m_customClipPath = QPainterPath();
        
        // 添加所有点，相对于中心点
        if (!points.empty()) {
            m_customClipPath.moveTo(points[0] - center);
            for (size_t i = 1; i < points.size(); ++i) {
                m_customClipPath.lineTo(points[i] - center);
            }
            m_customClipPath.closeSubpath();
        }
        
        // 更新位置和尺寸信息
        setPos(center);
        m_size = bounds.size();
        m_topLeft = QPointF(-m_size.width()/2, -m_size.height()/2);
        
        // 更新缓存和图形显示
        invalidateCache();
        update();
        
        Logger::info(QString("RectangleGraphicItem::restoreFromPoints: 恢复为自定义形状，点数: %1")
                    .arg(points.size()));
        return;
    }
    
    // 对于简单矩形，从点集合中获取左上角和右下角
    QPointF topLeft = points[0];
    QPointF bottomRight = points[1];
    
    // 计算新的尺寸
    QSizeF newSize(
        qAbs(bottomRight.x() - topLeft.x()),
        qAbs(bottomRight.y() - topLeft.y())
    );
    
    // 更新图形项属性
    m_useCustomPath = false;  // 重置为普通矩形
    m_size = newSize;
    m_topLeft = QPointF(-newSize.width()/2, -newSize.height()/2);
    
    // 更新缓存和图形显示
    invalidateCache();
    update();
    
    Logger::info(QString("RectangleGraphicItem::restoreFromPoints: 恢复为矩形，尺寸: %1x%2")
                .arg(m_size.width())
                .arg(m_size.height()));
}

// 重写绘制方法，支持自定义路径绘制
void RectangleGraphicItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    // 使用自定义路径绘制非矩形形状
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        // 设置高质量渲染选项
        painter->setRenderHint(QPainter::Antialiasing);
        
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
        
        // 如果被选中，绘制选择框
        if (isSelected()) {
            drawSelectionHandles(painter);
        }
        return;
    }
    
    // 默认使用父类绘制矩形
    GraphicItem::paint(painter, option, widget);
} 