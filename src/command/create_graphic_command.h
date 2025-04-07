#ifndef CREATE_GRAPHIC_COMMAND_H
#define CREATE_GRAPHIC_COMMAND_H

#include "command.h"
#include "../core/graphic.h"
#include <QGraphicsItem>
#include <QPointF>
#include <vector>

class DrawArea;

/**
 * @brief 创建图形命令类
 * 
 * 处理图形创建操作的撤销/重做功能
 */
class CreateGraphicCommand : public Command {
public:
    /**
     * @brief 构造函数
     * @param drawArea 绘图区域
     * @param type 图形类型
     * @param points 图形的点集合
     * @param pen 图形的画笔
     * @param brush 图形的画刷
     */
    CreateGraphicCommand(DrawArea* drawArea, 
                        Graphic::GraphicType type,
                        const std::vector<QPointF>& points,
                        const QPen& pen,
                        const QBrush& brush);
    
    /**
     * @brief 析构函数
     */
    ~CreateGraphicCommand() override;
    
    /**
     * @brief 执行创建图形命令
     */
    void execute() override;
    
    /**
     * @brief 撤销创建图形命令
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
     * @brief 获取创建的图形项
     * @return 图形项指针
     */
    QGraphicsItem* getCreatedItem() const { return m_createdItem; }

private:
    DrawArea* m_drawArea;
    Graphic::GraphicType m_type;
    std::vector<QPointF> m_points;
    QPen m_pen;
    QBrush m_brush;
    QGraphicsItem* m_createdItem = nullptr; // 创建的图形项
    bool m_executed = false;               // 命令是否已执行
};

#endif // CREATE_GRAPHIC_COMMAND_H 