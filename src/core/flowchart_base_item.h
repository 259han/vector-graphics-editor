#ifndef FLOWCHART_BASE_ITEM_H
#define FLOWCHART_BASE_ITEM_H

#include "graphic_item.h"
#include <QPainter>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QInputDialog>
#include <QApplication>
#include <QUuid>

/**
 * @brief 流程图图元基类
 * 
 * 实现所有流程图图元共有的功能，如文本处理、连接点管理等
 */
class FlowchartBaseItem : public GraphicItem {
public:
    FlowchartBaseItem();
    virtual ~FlowchartBaseItem() = default;
    
    // 文本处理
    void setText(const QString& text) { m_text = text; update(); }
    QString getText() const { return m_text; }
    void setTextVisible(bool visible) { m_textVisible = visible; update(); }
    bool isTextVisible() const { return m_textVisible; }
    void setTextFont(const QFont& font) { m_textFont = font; update(); }
    QFont getTextFont() const { return m_textFont; }
    void setTextColor(const QColor& color) { m_textColor = color; update(); }
    QColor getTextColor() const { return m_textColor; }
    
    // ID处理
    void setId(const QString& id) { m_id = id; }
    QString id() const { return m_id; }
    
    // UUID处理
    QUuid uuid() const { return m_uuid; }
    void setUuid(const QUuid& uuid) { m_uuid = uuid; }
    
    // 连接点处理
    virtual std::vector<QPointF> getConnectionPoints() const override;
    QPointF connectionPoint(int index) const;
    
    // 序列化和反序列化
    virtual void serialize(QDataStream& out) const override;
    virtual void deserialize(QDataStream& in) override;

protected:
    // 文本处理
    QString m_text;
    bool m_textVisible = false;
    QFont m_textFont = QFont("Arial", 10);
    QColor m_textColor = Qt::black;
    
    // ID
    QString m_id;
    
    // UUID
    QUuid m_uuid = QUuid::createUuid();
    
    // 鼠标拖动相关
    QPointF m_lastMousePos;
    
    // 绘制文本的辅助方法
    void drawText(QPainter* painter, const QRectF& rect);
    
    // 处理双击事件以编辑文本
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    
    // 处理鼠标悬停事件
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    
    // 处理鼠标事件
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    
    // 计算标准连接点位置
    virtual std::vector<QPointF> calculateConnectionPoints() const;
};

#endif // FLOWCHART_BASE_ITEM_H 