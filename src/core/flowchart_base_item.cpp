#include "flowchart_base_item.h"
#include "../utils/logger.h"
#include <QApplication>
#include <QGraphicsSceneHoverEvent>

FlowchartBaseItem::FlowchartBaseItem()
    : GraphicItem()
{
    // 设置默认属性
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    
    // 确保可移动性
    setMovable(true);
    
    // 确保接受鼠标悬停事件
    setAcceptHoverEvents(true);
    
    // 默认显示文本
    m_textVisible = true;
    m_text = "双击编辑文本";
}

// 悬停进入事件处理
void FlowchartBaseItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // 只有在可移动状态下才改变光标
    if (isMovable() && (flags() & ItemIsMovable)) {
        // 设置鼠标光标为移动光标
        QApplication::setOverrideCursor(Qt::SizeAllCursor);
        Logger::debug(QString("FlowchartBaseItem: 鼠标进入图元，设置移动光标 - 图元类型: %1").arg(getGraphicType()));
    }
    
    // 调用父类方法确保事件正确传递
    GraphicItem::hoverEnterEvent(event);
}

// 悬停离开事件处理
void FlowchartBaseItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    // 恢复默认光标
    while (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
        Logger::debug("FlowchartBaseItem: 鼠标离开图元，恢复默认光标");
    }
    
    // 调用父类方法确保事件正确传递
    GraphicItem::hoverLeaveEvent(event);
}

void FlowchartBaseItem::drawText(QPainter* painter, const QRectF& rect)
{
    if (!m_textVisible || m_text.isEmpty())
        return;
    
    // 保存画笔状态
    painter->save();
    
    // 设置文本绘制属性
    painter->setFont(m_textFont);
    painter->setPen(m_textColor);
    
    // 计算文本区域，考虑到旋转和缩放
    QRectF textRect = rect;
    
    // 居中绘制文本
    painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_text);
    
    // 恢复画笔状态
    painter->restore();
}

void FlowchartBaseItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    // 处理双击事件，弹出文本编辑对话框
    bool ok;
    QString newText = QInputDialog::getText(
        QApplication::activeWindow(),
        "编辑文本",
        "请输入文本内容:",
        QLineEdit::Normal,
        m_text,
        &ok
    );
    
    if (ok && !newText.isEmpty()) {
        m_text = newText;
        m_textVisible = true;
        update();
        Logger::debug(QString("流程图图元文本已更新：%1").arg(m_text));
    }
    
    event->accept();
}

std::vector<QPointF> FlowchartBaseItem::getConnectionPoints() const
{
    // 调用子类实现的计算方法
    return calculateConnectionPoints();
}

std::vector<QPointF> FlowchartBaseItem::calculateConnectionPoints() const
{
    // 基类实现默认的连接点：上、右、下、左四个方向的中点
    std::vector<QPointF> points;
    QRectF rect = boundingRect();
    
    // 上、右、下、左四个方向的中点
    points.push_back(QPointF(rect.center().x(), rect.top()));
    points.push_back(QPointF(rect.right(), rect.center().y()));
    points.push_back(QPointF(rect.center().x(), rect.bottom()));
    points.push_back(QPointF(rect.left(), rect.center().y()));
    
    // 转换为场景坐标
    for (auto& point : points) {
        point = mapToScene(point);
    }
    
    return points;
}

void FlowchartBaseItem::serialize(QDataStream& out) const
{
    // 先调用基类的序列化
    GraphicItem::serialize(out);
    Logger::debug("FlowchartBaseItem::serialize: 序列化");
    
    // 保存文本相关属性
    out << m_textVisible;
    out << m_text;
    out << m_textFont;
    out << m_textColor;
    Logger::debug(QString("FlowchartBaseItem::serialize: 文本='%1', 可见=%2, 字体=%3, 颜色=%4")
        .arg(m_text)
        .arg(m_textVisible)
        .arg(m_textFont.toString())
        .arg(m_textColor.name()));
    
    // 保存ID
    out << m_id;
    Logger::debug(QString("FlowchartBaseItem::serialize: ID='%1'").arg(m_id));
    
    // 保存UUID
    out << m_uuid;
    Logger::debug(QString("FlowchartBaseItem::serialize: UUID='%1'").arg(m_uuid.toString()));
    
    // 记录边界矩形信息
    QRectF rect = boundingRect();
    Logger::debug(QString("FlowchartBaseItem::serialize: 边界矩形=(%1, %2, %3, %4)")
        .arg(rect.left()).arg(rect.top())
        .arg(rect.width()).arg(rect.height()));
}

void FlowchartBaseItem::deserialize(QDataStream& in)
{
    // 先调用基类的反序列化
    GraphicItem::deserialize(in);
    Logger::debug(QString("FlowchartBaseItem::deserialize: 反序列化 this=%1").arg((quintptr)this));
    
    // 读取文本相关属性
    in >> m_textVisible;
    in >> m_text;
    in >> m_textFont;
    in >> m_textColor;
    Logger::debug(QString("FlowchartBaseItem::deserialize: 文本='%1', 可见=%2, 字体=%3, 颜色=%4")
        .arg(m_text)
        .arg(m_textVisible)
        .arg(m_textFont.toString())
        .arg(m_textColor.name()));
    
    // 读取ID
    in >> m_id;
    Logger::debug(QString("FlowchartBaseItem::deserialize: ID='%1'").arg(m_id));
    
    // 读取UUID
    in >> m_uuid;
    Logger::debug(QString("FlowchartBaseItem::deserialize: UUID='%1'").arg(m_uuid.toString()));
    
    // 记录边界矩形信息
    QRectF rect = boundingRect();
    Logger::debug(QString("FlowchartBaseItem::deserialize: 边界矩形=(%1, %2, %3, %4)")
        .arg(rect.left()).arg(rect.top())
        .arg(rect.width()).arg(rect.height()));
}

// 鼠标按下事件处理
void FlowchartBaseItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Logger::debug(QString("FlowchartBaseItem: 鼠标按下事件 - 图元类型: %1，位置：(%2, %3)，flags: %4，movable: %5")
                .arg(getGraphicType())
                .arg(event->scenePos().x()).arg(event->scenePos().y())
                .arg(flags())
                .arg(isMovable()));
    
    // 强制确保图形项可移动
    setMovable(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    
    // 记录初始点击位置
    m_lastMousePos = event->scenePos();
    
    // 确保图形项被选中
    setSelected(true);
    
    // 调用父类方法确保事件正确传递
    GraphicItem::mousePressEvent(event);
}

// 鼠标移动事件处理
void FlowchartBaseItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // 确保图元可移动
    if (!isMovable() || !(flags() & ItemIsMovable)) {
        Logger::debug(QString("FlowchartBaseItem: 强制启用移动功能 - 图元类型: %1").arg(getGraphicType()));
        setMovable(true);
        setFlag(QGraphicsItem::ItemIsMovable, true);
    }
    
    // 只处理左键拖动
    if (event->buttons() & Qt::LeftButton) {
        // 计算偏移量
        QPointF newPos = event->scenePos();
        QPointF delta = newPos - m_lastMousePos;
        
        Logger::debug(QString("FlowchartBaseItem: 鼠标移动 - 图元类型: %1，新位置：(%2, %3)，偏移：(%4, %5)")
                    .arg(getGraphicType())
                    .arg(newPos.x()).arg(newPos.y())
                    .arg(delta.x()).arg(delta.y()));
        
        // 直接使用QGraphicsItem的位置更新
        setPos(pos() + delta);
        
        // 更新最后位置
        m_lastMousePos = newPos;
        
        // 事件已处理
        event->accept();
        return;
    }
    
    // 调用父类方法处理其他情况
    GraphicItem::mouseMoveEvent(event);
}

// 鼠标释放事件处理
void FlowchartBaseItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Logger::debug(QString("FlowchartBaseItem: 鼠标释放 - 图元类型: %1，位置：(%2, %3)")
                .arg(getGraphicType())
                .arg(event->scenePos().x()).arg(event->scenePos().y()));
    
    // 调用父类方法确保事件正确传递
    GraphicItem::mouseReleaseEvent(event);
}

QPointF FlowchartBaseItem::connectionPoint(int index) const
{
    std::vector<QPointF> points = getConnectionPoints();
    if (index >= 0 && index < static_cast<int>(points.size())) {
        return points[index];
    }
    return QPointF();
}

void FlowchartBaseItem::restoreFromPoints(const std::vector<QPointF>& points)
{
    Logger::debug("FlowchartBaseItem::restoreFromPoints: 开始恢复图形形状和大小");

    if (points.empty()) {
        Logger::warning("FlowchartBaseItem::restoreFromPoints: 点集为空，无法恢复形状和大小");
        return;
    }

    // 如果只有1个点，则将其视为中心点
    if (points.size() == 1) {
        setPos(points[0]);
        Logger::debug("FlowchartBaseItem::restoreFromPoints: 恢复图形位置成功(1点)");
    } else {
        // 直接使用第一个点作为中心点
        setPos(points[0]);
        
        Logger::debug(QString("FlowchartBaseItem::restoreFromPoints: 恢复图形位置成功(多点) - 中心=(%1,%2)")
            .arg(points[0].x()).arg(points[0].y()));
    }
    
    invalidateCache();
    update();
}

std::vector<QPointF> FlowchartBaseItem::getClipboardPoints() const
{
    QPointF center = scenePos(); // 图元中心
    QSizeF size = boundingRect().size(); // 图元大小（局部坐标系）
    
    QPointF sizePoint = center + QPointF(size.width() / 2, size.height() / 2);
    
    Logger::debug(QString("FlowchartBaseItem::getClipboardPoints: 中心=(%1, %2), 大小=(%3, %4), 大小点=(%5, %6)")
        .arg(center.x()).arg(center.y())
        .arg(size.width()).arg(size.height())
        .arg(sizePoint.x()).arg(sizePoint.y()));
    
    return {center, sizePoint};
} 