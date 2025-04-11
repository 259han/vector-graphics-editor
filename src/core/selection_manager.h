#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <QGraphicsItem>
#include <QPointF>
#include <QPainterPath>
#include <QList>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QObject>
#include <QPainter>
#include <QSet>
#include <QGraphicsView>
#include <functional>
#include "../core/graphic_item.h"

class GraphicItem;

// 选择区域管理器类，负责处理选择区域的创建、显示和交互
class SelectionManager : public QObject {
    Q_OBJECT
    
public:
    // 选择模式枚举
    enum SelectionMode {
        SingleSelection,    // 单选模式
        MultiSelection,     // 多选模式（保持之前的选择）
        RectSelection,      // 矩形区域选择
    };
    
    // 选择过滤器接口
    using SelectionFilter = std::function<bool(QGraphicsItem*)>;
    
    explicit SelectionManager(QGraphicsScene* scene = nullptr);
    ~SelectionManager();
    
    // 开始创建选择区域
    void startSelection(const QPointF& startPoint, SelectionMode mode = SingleSelection);
    
    // 更新选择区域
    void updateSelection(const QPointF& currentPoint);
    
    // 完成选择操作
    void finishSelection();
    
    // 清除选择区域
    void clearSelection();
    
    // 获取选择区域
    QRectF getSelectionRect() const;
    
    // 获取选择区域路径
    QPainterPath getSelectionPath() const;
    
    // 检查选择区域是否有效
    bool isSelectionValid() const;
    
    // 设置选择区域的外观
    void setSelectionAppearance(const QPen& pen, const QBrush& brush);
    
    // 检查点是否在选择区域内
    bool contains(const QPointF& point) const;
    
    // 处理选择区域的移动
    void moveSelection(const QPointF& offset);
    
    // 获取选择区域中的所有图形项
    QList<QGraphicsItem*> getSelectedItems() const;
    
    // 设置是否正在拖动选择区域
    void setDraggingSelection(bool dragging) { m_isDraggingSelection = dragging; }
    
    // 判断是否正在拖动选择区域
    bool isDraggingSelection() const { return m_isDraggingSelection; }
    
    // 设置场景
    void setScene(QGraphicsScene* scene);
    
    // 获取场景
    QGraphicsScene* scene() const { return m_scene; }
    
    // 检查点是否在控制点上，如果是则返回控制点类型
    GraphicItem::ControlHandle handleAtPoint(const QPointF& point) const;
    
    // 缩放选择区域
    void scaleSelection(GraphicItem::ControlHandle handle, const QPointF& point);
    
    // 绘制选择区域和控制点
    void paint(QPainter* painter, const QGraphicsView* view);
    
    // 设置选择模式
    void setSelectionMode(SelectionMode mode) { m_selectionMode = mode; }
    SelectionMode getSelectionMode() const { return m_selectionMode; }
    
    // 设置选择过滤器
    void setSelectionFilter(const SelectionFilter& filter) { m_filter = filter; }
    void clearSelectionFilter() { m_filter = nullptr; }
    
    // 添加到选择
    void addToSelection(QGraphicsItem* item);
    
    // 从选择中移除
    void removeFromSelection(QGraphicsItem* item);
    
    // 切换选择状态
    void toggleSelection(QGraphicsItem* item);
    
    // 判断某项是否被选中
    bool isSelected(QGraphicsItem* item) const;
    
    // 获取选择列表
    const QSet<QGraphicsItem*>& selectionSet() const { return m_selectedItems; }
    
    // 获取选择中心点
    QPointF selectionCenter() const;
    
    // 应用选择到场景
    void applySelectionToScene();
    
    // 从场景同步选择状态
    void syncSelectionFromScene();
    
signals:
    // 选择改变信号
    void selectionChanged();
    
    // 选择完成信号
    void selectionFinished();
    
    // 选择开始信号
    void selectionStarted();
    
private:
    QGraphicsScene* m_scene;
    QGraphicsRectItem* m_selectionRect;
    QPointF m_startPoint;
    QPointF m_currentPoint;
    bool m_isDraggingSelection;
    SelectionMode m_selectionMode;
    SelectionFilter m_filter;
    QSet<QGraphicsItem*> m_selectedItems;  // 使用集合存储选中的项
    QSet<QGraphicsItem*> m_previousSelection; // 用于保存多选模式下的之前选择
    
    // 更新选择区域的外观
    void updateSelectionAppearance();
    
    // 应用过滤器
    bool applyFilter(QGraphicsItem* item) const;
    
    // 更新选择
    void updateSelectionFromRect();
};

#endif // SELECTION_MANAGER_H 