#include "fill_command.h"
#include "../ui/draw_area.h"
#include "../utils/logger.h"
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
    
    // 将整个场景渲染到图像中进行填充
    QImage image(qCeil(sceneRect.width()), qCeil(sceneRect.height()), QImage::Format_ARGB32);
    image.fill(Qt::transparent);  // 使用透明背景
    
    QPainter painter(&image);
    // 禁用抗锯齿，这样边界更明确
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // 调整坐标系，使场景左上角对应图像的(0,0)
    painter.translate(-sceneRect.topLeft());
    
    // 渲染场景到图像 - 确保渲染所有图层
    m_drawArea->scene()->render(&painter, sceneRect, sceneRect, Qt::KeepAspectRatio);
    painter.end();
    
    // 将场景坐标转换为图像坐标
    QPoint imagePoint(
        static_cast<int>(m_position.x() - sceneRect.left()),
        static_cast<int>(m_position.y() - sceneRect.top())
    );
    
    // 检查图像坐标是否在有效范围内
    if (imagePoint.x() < 0 || imagePoint.y() < 0 || 
        imagePoint.x() >= image.width() || imagePoint.y() >= image.height()) {
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
    
    // 执行填充算法
    QStack<QPoint> stack;
    stack.push(imagePoint);
    
    int width = image.width();
    int height = image.height();
    QVector<QVector<bool>> visited(width, QVector<bool>(height, false));
    
    m_filledPixelsCount = 0;
    
    while (!stack.isEmpty()) {
        QPoint point = stack.pop();
        int x = point.x();
        int y = point.y();
        
        // 如果点已访问过或颜色不匹配，跳过
        if (x < 0 || x >= width || y < 0 || y >= height || 
            visited[x][y] || image.pixelColor(x, y) != targetColor) {
            continue;
        }
        
        // 向左找到边界
        int left = x;
        while (left >= 0 && image.pixelColor(left, y) == targetColor && !visited[left][y]) {
            left--;
        }
        left++;
        
        // 向右找到边界
        int right = x;
        while (right < width && image.pixelColor(right, y) == targetColor && !visited[right][y]) {
            right++;
        }
        right--;
        
        // 填充当前行
        bool spanAbove = false;
        bool spanBelow = false;
        
        for (int i = left; i <= right; i++) {
            visited[i][y] = true;
            fillImage.setPixelColor(i, y, m_color);
            m_filledPixelsCount++;
            
            // 检查上一行
            if (y > 0) {
                QColor aboveColor = image.pixelColor(i, y - 1);
                bool aboveMatch = aboveColor == targetColor && !visited[i][y-1];
                
                if (!spanAbove && aboveMatch) {
                    stack.push(QPoint(i, y - 1));
                    spanAbove = true;
                } else if (spanAbove && !aboveMatch) {
                    spanAbove = false;
                }
            }
            
            // 检查下一行
            if (y < height - 1) {
                QColor belowColor = image.pixelColor(i, y + 1);
                bool belowMatch = belowColor == targetColor && !visited[i][y+1];
                
                if (!spanBelow && belowMatch) {
                    stack.push(QPoint(i, y + 1));
                    spanBelow = true;
                } else if (spanBelow && !belowMatch) {
                    spanBelow = false;
                }
            }
        }
    }
    
    if (m_filledPixelsCount > 0) {
        // 创建新的图层，仅包含填充的像素
        QImage resultImage(image.size(), QImage::Format_ARGB32);
        resultImage.fill(Qt::transparent);
        
        // 只复制被填充的像素
        for (int y = 0; y < image.height(); y++) {
            for (int x = 0; x < image.width(); x++) {
                if (fillImage.pixelColor(x, y) == m_color &&
                    image.pixelColor(x, y) != m_color) {
                    resultImage.setPixelColor(x, y, m_color);
                }
            }
        }
        
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