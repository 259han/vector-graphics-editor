#ifndef STYLE_CHANGE_COMMAND_H
#define STYLE_CHANGE_COMMAND_H

#include "command.h"
#include <QList>
#include <QGraphicsItem>
#include <QPen>
#include <QBrush>

class DrawArea;
class GraphicItem;

/**
 * @brief 样式变更命令类
 * 
 * 处理图形项样式变更操作的撤销/重做功能
 */
class StyleChangeCommand : public Command {
public:
    enum StylePropertyType {
        PenStyle,       // 画笔样式
        PenWidth,       // 画笔宽度
        PenColor,       // 画笔颜色
        BrushColor,     // 画刷颜色
        BrushStyle      // 画刷样式
    };
    
    /**
     * @brief 构造函数
     * @param drawArea 绘图区域
     * @param items 要变更样式的图形项列表
     * @param propertyType 要变更的样式属性
     */
    StyleChangeCommand(DrawArea* drawArea, 
                      const QList<QGraphicsItem*>& items,
                      StylePropertyType propertyType);
    
    /**
     * @brief 析构函数
     */
    ~StyleChangeCommand() override;
    
    /**
     * @brief 执行样式变更命令
     */
    void execute() override;
    
    /**
     * @brief 撤销样式变更命令
     */
    void undo() override;
    
    /**
     * @brief 获取命令描述
     * @return 命令描述字符串
     */
    QString getDescription() const override;
    
    /**
     * @brief 获取命令类型
     * @return 命令类型字符串
     */
    QString getType() const override;
    
    /**
     * @brief 设置新的画笔
     */
    void setNewPen(const QPen& pen);
    
    /**
     * @brief 设置新的画刷
     */
    void setNewBrush(const QBrush& brush);
    
    /**
     * @brief 设置新的画笔宽度
     */
    void setNewPenWidth(qreal width);
    
    /**
     * @brief 设置新的画笔颜色
     */
    void setNewPenColor(const QColor& color);
    
    /**
     * @brief 设置新的画刷颜色
     */
    void setNewBrushColor(const QColor& color);

private:
    struct ItemState {
        GraphicItem* item;
        QPen oldPen;
        QBrush oldBrush;
    };
    
    DrawArea* m_drawArea;
    QList<ItemState> m_itemStates;
    StylePropertyType m_propertyType;
    
    // 新样式属性
    QPen m_newPen;
    QBrush m_newBrush;
    qreal m_newPenWidth = 1.0;
    QColor m_newPenColor = Qt::black;
    QColor m_newBrushColor = Qt::transparent;
    
    bool m_executed = false;
    
    // 保存图形项的当前样式状态
    void saveItemStyles(const QList<QGraphicsItem*>& items);
};

#endif // STYLE_CHANGE_COMMAND_H 