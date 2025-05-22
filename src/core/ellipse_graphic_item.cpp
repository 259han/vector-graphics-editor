#include "ellipse_graphic_item.h"
#include "../utils/logger.h"
#include "../utils/clip_algorithms.h"
#include <QPainter>

EllipseGraphicItem::EllipseGraphicItem(const QPointF& center, double width, double height)
    : m_center(QPointF(0, 0)), m_width(std::max(1.0, width)), m_height(std::max(1.0, height))
{
    // 设置绘制策略为EllipseDrawStrategy
    m_drawStrategy = std::make_shared<EllipseDrawStrategy>();
    
    // 设置位置为椭圆中心
    setPos(center);
    
    // 设置默认画笔和画刷
    m_pen.setColor(Qt::black);
    m_pen.setWidth(1);
    m_brush = Qt::NoBrush; // 无填充
}

QRectF EllipseGraphicItem::boundingRect() const
{
    // 如果使用自定义路径，使用自定义路径的边界
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        // 获取自定义路径的边界矩形
        QRectF pathBounds = m_customClipPath.boundingRect();
        
        // 增加一些边距确保能正确显示边框
        qreal extra = m_pen.width() + 2.0;
        return pathBounds.adjusted(-extra, -extra, extra, extra);
    }
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_width * m_scale.x();
    double scaledHeight = m_height * m_scale.y();
    
    // 边界矩形，确保包含整个椭圆
    qreal extra = m_pen.width() + 2.0; // 额外的边距
    return QRectF(
        -scaledWidth/2 - extra,
        -scaledHeight/2 - extra,
        scaledWidth + extra * 2,
        scaledHeight + extra * 2
    );
}

// 添加shape方法实现准确的椭圆碰撞检测
QPainterPath EllipseGraphicItem::shape() const
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
    double scaledWidth = m_width * m_scale.x();
    double scaledHeight = m_height * m_scale.y();
    
    // 创建一个椭圆形状的路径
    QPainterPath path;
    path.addEllipse(QRectF(
        -scaledWidth/2, 
        -scaledHeight/2,
        scaledWidth,
        scaledHeight
    ));
    
    // 考虑画笔宽度的影响，使用strokePath扩展路径
    QPainterPathStroker stroker;
    stroker.setWidth(m_pen.width());
    return path.united(stroker.createStroke(path));
}

// 重写contains方法，实现更准确的椭圆内部检测
bool EllipseGraphicItem::contains(const QPointF& point) const
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
    
    // 将点从场景坐标转换为图形项的本地坐标
    QPointF localPoint = mapFromScene(point);
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_width * m_scale.x();
    double scaledHeight = m_height * m_scale.y();
    
    // 椭圆方程: (x/a)² + (y/b)² <= 1 表示点在椭圆内部
    double a = scaledWidth / 2.0;
    double b = scaledHeight / 2.0;
    
    // 计算点到椭圆中心的归一化距离
    double normalizedDistance = (localPoint.x() * localPoint.x()) / (a * a) + 
                               (localPoint.y() * localPoint.y()) / (b * b);
    
    // 如果距离小于等于1，点在椭圆内部
    // 为了考虑画笔宽度，增加一个较大的容差
    double tolerance = (m_pen.width() / qMin(a, b)) + 0.1;
    
    return normalizedDistance <= (1.0 + tolerance);
}

std::vector<QPointF> EllipseGraphicItem::getDrawPoints() const
{
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_width * m_scale.x();
    double scaledHeight = m_height * m_scale.y();
    
    // 提供给DrawStrategy的点集合
    // 对于椭圆，创建左上角和右下角点来定义包围矩形
    QPointF topLeft(-scaledWidth/2, -scaledHeight/2);  // 左上角点
    QPointF bottomRight(scaledWidth/2, scaledHeight/2);  // 右下角点
    
    // 确保返回标准化矩形的点，与预览中创建的QRectF匹配
    return { topLeft, bottomRight };
}

QPointF EllipseGraphicItem::getCenter() const
{
    // 返回全局坐标系中的中心点
    return pos();
}

void EllipseGraphicItem::setCenter(const QPointF& center)
{
    // 设置新的位置（中心点）
    setPos(center);
}

double EllipseGraphicItem::getWidth() const
{
    return m_width;
}

void EllipseGraphicItem::setWidth(double width)
{
    // 确保宽度有效
    m_width = std::max(1.0, width);
    update();
}

double EllipseGraphicItem::getHeight() const
{
    return m_height;
}

void EllipseGraphicItem::setHeight(double height)
{
    // 确保高度有效
    m_height = std::max(1.0, height);
    update();
}

void EllipseGraphicItem::setSize(double width, double height)
{
    // 确保尺寸有效
    m_width = std::max(1.0, width);
    m_height = std::max(1.0, height);
    update();
}

void EllipseGraphicItem::setScale(const QPointF& scale)
{
    // 设置新的基础比例
    m_scale = scale;
    
    // 直接应用scale进行绘制，而不是改变m_width和m_height
    // 这样可以避免累积放大效应
    
    // 绘制时的宽高会根据getDrawPoints()计算，因此不需要修改m_width和m_height
    
    // 设置QGraphicsItem的基础缩放为1.0，让我们自行处理缩放
    QGraphicsItem::setScale(1.0);
    
    Logger::debug(QString("EllipseGraphicItem::setScale - 设置为(%1, %2), 基础尺寸: %3x%4")
                 .arg(scale.x(), 0, 'f', 3)
                 .arg(scale.y(), 0, 'f', 3)
                 .arg(m_width)
                 .arg(m_height));
    
    // 更新图形
    update();
}

void EllipseGraphicItem::setScale(qreal scale)
{
    // 使用相同的x和y缩放
    setScale(QPointF(scale, scale));
}

bool EllipseGraphicItem::clip(const QPainterPath& clipPath)
{
    Logger::debug("EllipseGraphicItem::clip: 开始执行椭圆裁剪");

    // 获取椭圆的当前边界
    QRectF bounds = boundingRect();
    bounds.translate(pos());  // 转换为场景坐标
    Logger::debug(QString("EllipseGraphicItem::clip: 原始形状边界: (%1,%2,%3,%4)")
        .arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height()));

    // 将裁剪路径转换为点集
    std::vector<QPointF> clipPoints = ClipAlgorithms::pathToPoints(clipPath, 0.5);
    Logger::debug(QString("EllipseGraphicItem::clip: 裁剪路径点数: %1").arg(clipPoints.size()));

    // 获取椭圆的路径
    QPainterPath path = toPath();
    // 转换到场景坐标
    QTransform toScene;
    toScene.translate(pos().x(), pos().y());
    path = toScene.map(path);

    // 使用裁剪算法计算路径交集
    QPainterPath resultPath = ClipAlgorithms::clipPath(path, clipPath);
    Logger::debug(QString("EllipseGraphicItem::clip: 裁剪结果元素数: %1, 点数: %2")
        .arg(resultPath.elementCount())
        .arg(ClipAlgorithms::pathToPoints(resultPath, 0.5).size()));

    // 检查结果是否为空
    if (resultPath.isEmpty()) {
        Logger::warning("EllipseGraphicItem::clip: 裁剪结果为空，没有交集");
        return false;
    }

    // 保存当前可移动状态
    bool wasMovable = isMovable();

    // 判断裁剪结果是否仍是椭圆
    // 这个判断比较复杂，我们可以通过检查结果路径的点数和边界来简单估计
    QRectF resultBounds = resultPath.boundingRect();
    std::vector<QPointF> resultPoints = ClipAlgorithms::pathToPoints(resultPath, 0.5);
    
    // 如果裁剪结果点数很少，且边界接近矩形，可能是椭圆
    bool isStillEllipse = resultPoints.size() < 20 && 
                         qAbs(resultBounds.width() - bounds.width()) < 10 && 
                         qAbs(resultBounds.height() - bounds.height()) < 10;
    
    if (isStillEllipse) {
        // 如果结果仍是椭圆形状，调整椭圆大小和位置
        setPos(resultBounds.center());
        
        // 将尺寸设置为新边界的尺寸
        setSize(resultBounds.width(), resultBounds.height());
        
        // 确保不使用自定义路径
        m_useCustomPath = false;
        
        // 更新缓存和图形显示
        invalidateCache();
        update();
        
        Logger::info(QString("EllipseGraphicItem::clip: 裁剪结果仍是椭圆，尺寸: %1x%2")
                    .arg(resultBounds.width())
                    .arg(resultBounds.height()));
    } else {
        // 非椭圆裁剪结果，转换为自定义形状
        Logger::debug(QString("EllipseGraphicItem::clip: 裁剪结果不是椭圆，点数: %1")
                     .arg(resultPoints.size()));
        
        if (resultPoints.size() < 3) {
            Logger::warning("EllipseGraphicItem::clip: 裁剪结果点数不足，无法创建有效形状");
            return false;
        }
        
        // 计算结果边界和中心点
        QPointF newCenter = resultBounds.center();
        
        // 设置图形项位置为裁剪结果的中心
        setPos(newCenter);
        
        // 将裁剪结果转换为相对于新中心点的坐标
        QTransform transform;
        transform.translate(-newCenter.x(), -newCenter.y());
        m_customClipPath = transform.map(resultPath);
        
        // 最后再次检查自定义路径是否有效
        if (m_customClipPath.isEmpty()) {
            Logger::warning("EllipseGraphicItem::clip: 转换后的自定义路径为空，保持原图形不变");
            return false;
        }
        
        // 保留WindingFill作为计算规则
        m_customClipPath.setFillRule(Qt::WindingFill);
        
        // 启用自定义路径绘制模式
        m_useCustomPath = true;
        
        // 保存尺寸信息（主要用于边界框计算）
        m_width = resultBounds.width();
        m_height = resultBounds.height();
        
        // 如果点太多，尝试简化点集以提高性能
        if (resultPoints.size() > 100) {
            Logger::debug("EllipseGraphicItem::clip: 尝试简化过多的点");
            // 使用更大的flatness值重新生成路径点，减少点数
            std::vector<QPointF> simplifiedPoints = ClipAlgorithms::pathToPoints(m_customClipPath, 1.0);
            if (simplifiedPoints.size() >= 3 && simplifiedPoints.size() < resultPoints.size()) {
                Logger::debug(QString("EllipseGraphicItem::clip: 成功简化点数从 %1 到 %2")
                             .arg(resultPoints.size())
                             .arg(simplifiedPoints.size()));
                
                // 重新创建简化后的路径
                m_customClipPath = ClipAlgorithms::pointsToPath(simplifiedPoints);
                m_customClipPath.setFillRule(Qt::WindingFill);
                resultPoints = simplifiedPoints;
            }
        }
        
        // 更新缓存和图形显示
        invalidateCache();
        update();
        
        Logger::info(QString("EllipseGraphicItem::clip: 裁剪完成，转换为自定义形状，点数: %1")
                    .arg(resultPoints.size()));
    }
    
    // 确保裁剪后保持可移动状态
    setMovable(wasMovable);
    
    // 手动设置图形项标志，确保可以接收鼠标事件和移动
    setFlag(QGraphicsItem::ItemIsMovable, wasMovable);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    
    Logger::debug(QString("EllipseGraphicItem::clip: 设置可移动状态为 %1").arg(wasMovable ? "可移动" : "不可移动"));
    
    return true;
}

QPainterPath EllipseGraphicItem::toPath() const
{
    // 如果使用自定义路径，直接返回自定义路径
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        return m_customClipPath;
    }
    
    // 应用缩放因子计算实际尺寸
    double scaledWidth = m_width * m_scale.x();
    double scaledHeight = m_height * m_scale.y();
    
    // 创建椭圆路径（相对于图形项坐标系）
    QPainterPath path;
    path.addEllipse(QRectF(
        -scaledWidth/2,
        -scaledHeight/2,
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

void EllipseGraphicItem::restoreFromPoints(const std::vector<QPointF>& points)
{
    // 检查点集合大小
    if (points.size() < 2) {
        Logger::warning("EllipseGraphicItem::restoreFromPoints: 点集合不足，无法恢复");
        return;
    }
    
    // 如果点数大于4，可能是非椭圆形状，使用自定义路径
    if (points.size() > 4) {
        Logger::debug(QString("EllipseGraphicItem::restoreFromPoints: 检测到非椭圆形状，点数: %1")
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
        m_width = bounds.width();
        m_height = bounds.height();
        
        // 更新缓存和图形显示
        invalidateCache();
        update();
        
        Logger::info(QString("EllipseGraphicItem::restoreFromPoints: 恢复为自定义形状，点数: %1")
                    .arg(points.size()));
        return;
    }
    
    // 对于简单椭圆，从点集合中获取左上角和右下角
    QPointF topLeft = points[0];
    QPointF bottomRight = points[1];
    
    // 计算新的尺寸和中心点
    double newWidth = qAbs(bottomRight.x() - topLeft.x());
    double newHeight = qAbs(bottomRight.y() - topLeft.y());
    
    // 更新图形项属性
    m_useCustomPath = false;  // 重置为普通椭圆
    m_width = newWidth;
    m_height = newHeight;
    
    // 更新缓存和图形显示
    invalidateCache();
    update();
    
    Logger::info(QString("EllipseGraphicItem::restoreFromPoints: 恢复为椭圆，尺寸: %1x%2")
                .arg(m_width)
                .arg(m_height));
}

void EllipseGraphicItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    // 使用自定义路径绘制非椭圆形状
    if (m_useCustomPath && !m_customClipPath.isEmpty()) {
        // 设置高质量渲染选项
        painter->setRenderHint(QPainter::Antialiasing);
        
        // 保存当前的画笔和画刷
        QPen oldPen = painter->pen();
        QBrush oldBrush = painter->brush();
        
        // 设置画笔和画刷
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        
        // 明确禁用填充，仅绘制轮廓
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
    
    // 默认使用父类绘制椭圆
    GraphicItem::paint(painter, option, widget);
} 