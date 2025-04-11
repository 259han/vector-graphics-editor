#ifndef TRANSFORM_COMMAND_H
#define TRANSFORM_COMMAND_H

#include "command.h"
#include <QGraphicsItem>
#include <QList>
#include <QPointF>
#include <QMap>

// 变换类型枚举
enum class TransformType {
    Rotate,
    Scale,
    Flip
};

// 变换命令类 - 用于处理多个图形项的变换操作
class TransformCommand : public Command {
public:
    static TransformCommand* createRotateCommand(QList<QGraphicsItem*> items, double angle, QPointF center);
    static TransformCommand* createScaleCommand(QList<QGraphicsItem*> items, double factor, QPointF center);
    static TransformCommand* createFlipCommand(QList<QGraphicsItem*> items, bool horizontal, QPointF center);
    
    ~TransformCommand() override = default;
    
    void execute() override;
    void undo() override;
    QString getDescription() const override;
    QString getType() const override;

private:
    // 私有构造函数，只能通过工厂方法创建
    TransformCommand(QList<QGraphicsItem*> items, TransformType type);
    
    // 用于记录变换前的状态
    struct ItemState {
        QPointF position;
        qreal rotation;
        QPointF scale;
    };
    
    QList<QGraphicsItem*> m_items;
    QMap<QGraphicsItem*, ItemState> m_originalStates;
    TransformType m_transformType;
    double m_angle = 0.0;      // 用于旋转操作
    double m_factor = 1.0;     // 用于缩放操作
    bool m_isHorizontal = true; // 用于翻转操作
    QPointF m_center;          // 变换中心
    bool m_executed = false;
    
    // 设置参数
    void setRotationParams(double angle, QPointF center);
    void setScaleParams(double factor, QPointF center);
    void setFlipParams(bool horizontal, QPointF center);
    
    // 保存原始状态
    void saveOriginalStates();
    
    // 应用变换
    void applyRotation();
    void applyScaling();
    void applyFlip();
};

#endif // TRANSFORM_COMMAND_H 