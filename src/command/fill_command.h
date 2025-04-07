#ifndef FILL_COMMAND_H
#define FILL_COMMAND_H

#include "command.h"
#include <QPointF>
#include <QColor>
#include <QGraphicsPixmapItem>

class DrawArea;

/**
 * @brief 填充命令类
 * 
 * 处理填充操作的撤销/重做功能
 */
class FillCommand : public Command {
public:
    /**
     * @brief 构造函数
     * @param drawArea 绘图区域
     * @param position 填充的位置
     * @param color 填充的颜色
     */
    FillCommand(DrawArea* drawArea, const QPointF& position, const QColor& color);
    
    /**
     * @brief 析构函数
     */
    ~FillCommand() override;
    
    /**
     * @brief 执行填充命令
     */
    void execute() override;
    
    /**
     * @brief 撤销填充命令
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
     * @brief 设置填充的像素数量（用于调试）
     */
    void setFilledPixelsCount(int count) { m_filledPixelsCount = count; }
    
private:
    DrawArea* m_drawArea;
    QPointF m_position;
    QColor m_color;
    QGraphicsPixmapItem* m_fillItem = nullptr;
    int m_filledPixelsCount = 0;
    bool m_executed = false;
    
    // 执行实际的填充算法
    void doFill();
};

#endif // FILL_COMMAND_H 