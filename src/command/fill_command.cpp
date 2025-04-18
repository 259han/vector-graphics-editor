#include "fill_command.h"
#include "../ui/draw_area.h"
#include "../utils/logger.h"
#include "../utils/graphics_utils.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QImage>
#include <QStack>
#include <QPoint>

FillCommand::FillCommand(DrawArea* drawArea, const QPointF& position, const QColor& color)
    : m_drawArea(drawArea),
      m_position(position),
      m_color(color)
{
    Logger::debug(QString("FillCommand: 创建填充命令 - 位置: (%1, %2), 颜色: %3")
        .arg(position.x()).arg(position.y())
        .arg(color.name(QColor::HexArgb)));
}

FillCommand::~FillCommand()
{
    // 注意：不删除fillItem，因为它属于场景
    Logger::debug("FillCommand: 销毁填充命令");
}

void FillCommand::execute()
{
    if (m_executed || !m_drawArea) {
        return;
    }
    
    // 执行填充操作
    doFill();
    
    m_executed = true;
    Logger::info(QString("FillCommand: 执行填充命令 - 填充了 %1 个像素")
        .arg(m_filledPixelsCount));
}

void FillCommand::undo()
{
    if (!m_executed || !m_drawArea || !m_fillItem) {
        return;
    }
    
    // 从场景中移除填充项
    m_drawArea->scene()->removeItem(m_fillItem);
    m_executed = false;
    
    Logger::info("FillCommand: 撤销填充命令");
}

void FillCommand::doFill()
{
    // 获取场景矩形
    QRectF sceneRect = m_drawArea->scene()->sceneRect();
    
    // 使用GraphicsUtils渲染场景到图像，禁用抗锯齿以获得更清晰的边界
    QImage image = GraphicsUtils::renderSceneToImage(m_drawArea->scene(), true);
    
    // 将场景坐标转换为图像坐标
    QPoint imagePoint = GraphicsUtils::sceneToImageCoordinates(m_position, sceneRect);
    
    // 检查图像坐标是否在有效范围内
    if (!GraphicsUtils::isPointInImageBounds(imagePoint, image.width(), image.height())) {
        Logger::debug("FillCommand: 填充点不在有效图像范围内");
        return;
    }
    
    // 获取目标颜色（种子点的颜色）
    QColor targetColor = image.pixelColor(imagePoint);
    
    // 如果目标颜色与填充颜色相同，则无需填充
    if (targetColor == m_color) {
        Logger::debug("FillCommand: 目标颜色与填充颜色相同，无需填充");
        return;
    }
    
    // 创建一个副本用于填充，保留原始图像
    QImage fillImage = image;
    
    // 使用GraphicsUtils中的填充方法执行填充算法
    m_filledPixelsCount = GraphicsUtils::fillImageRegion(fillImage, imagePoint, targetColor, m_color);
    
    if (m_filledPixelsCount > 0) {
        // 使用GraphicsUtils创建填充结果图层
        QImage resultImage = GraphicsUtils::createFillResultLayer(image, fillImage, m_color);
        
        // 创建填充结果的图像项
        QPixmap fillResult = QPixmap::fromImage(resultImage);
        m_fillItem = new QGraphicsPixmapItem(fillResult);
        m_fillItem->setPos(sceneRect.topLeft());  // 与场景左上角对齐
        
        // 确保填充操作位于合适的Z轴位置
        m_fillItem->setZValue(-1);  // 置于图形下方
        
        // 添加到场景
        m_drawArea->scene()->addItem(m_fillItem);
        
        Logger::debug(QString("FillCommand: 填充完成，填充了 %1 个像素").arg(m_filledPixelsCount));
    } else {
        Logger::debug("FillCommand: 未填充任何像素");
    }
}

QString FillCommand::getDescription() const
{
    return QString("填充区域 (坐标: %1, %2 颜色: %3)")
            .arg(m_position.x())
            .arg(m_position.y())
            .arg(m_color.name());
}

QString FillCommand::getType() const
{
    return "fill";
} 