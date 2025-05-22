#ifndef CLIP_COMMAND_H
#define CLIP_COMMAND_H

#include "command.h"
#include "../core/graphic_item.h"
#include <QGraphicsItem>
#include <QPainterPath>
#include <vector>
#include <memory>
#include <QGraphicsScene>

/**
 * @brief 图形裁剪命令类
 * 
 * 处理图形裁剪操作的撤销/重做功能
 */
class ClipCommand : public Command {
public:
    /**
     * @brief 构造函数
     * @param scene 场景指针
     * @param item 要裁剪的图形项
     * @param clipPath 裁剪路径
     */
    ClipCommand(QGraphicsScene* scene, 
               GraphicItem* item,
               const QPainterPath& clipPath);
    
    /**
     * @brief 析构函数
     */
    ~ClipCommand() override;
    
    /**
     * @brief 执行裁剪命令
     */
    void execute() override;
    
    /**
     * @brief 撤销裁剪命令
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
    QGraphicsScene* m_scene;             // 场景指针
    GraphicItem* m_item;                 // 要裁剪的图形项
    QPainterPath m_clipPath;             // 裁剪路径
    bool m_executed = false;             // 命令是否已执行
    
    // 保存原始图形数据用于撤销
    std::vector<QPointF> m_originalPoints;
    QPainterPath m_originalPath;
};

#endif // CLIP_COMMAND_H 