#ifndef PASTE_COMMAND_H
#define PASTE_COMMAND_H

#include "command.h"
#include "../core/graphic_item.h"
#include <QGraphicsItem>
#include <QList>
#include <QPointF>

class DrawArea;

/**
 * @brief 粘贴图形命令类
 * 
 * 处理粘贴操作的撤销/重做功能，将多个图形作为一个组进行操作
 */
class PasteGraphicCommand : public Command {
public:
    /**
     * @brief 构造函数
     * @param drawArea 绘图区域
     * @param items 已粘贴的图形项列表（这些项目应该已经添加到场景中）
     */
    PasteGraphicCommand(DrawArea* drawArea, const QList<QGraphicsItem*>& items);
    
    /**
     * @brief 析构函数
     */
    ~PasteGraphicCommand() override;
    
    /**
     * @brief 执行粘贴命令（这些项目应该已经被添加到场景）
     */
    void execute() override;
    
    /**
     * @brief 撤销粘贴命令
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

private:
    DrawArea* m_drawArea;
    QList<QGraphicsItem*> m_pastedItems; // 粘贴的图形项
    bool m_executed; // 命令是否已执行
    
    // 记录每个图形项的原始状态，用于撤销
    struct ItemState {
        QGraphicsItem* item;
        QPointF position;
        bool isSelected;
    };
    
    QList<ItemState> m_itemStates;
    
    // 保存项目状态用于撤销
    void saveItemStates();
};

#endif // PASTE_COMMAND_H 