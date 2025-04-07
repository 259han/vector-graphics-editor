#include "fill_state.h"
#include "../ui/draw_area.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QDebug>
#include <QGraphicsScene>
#include <QApplication>
#include <QCursor>
#include <QGraphicsPixmapItem>
#include <QMainWindow>
#include <QStatusBar>
#include <QStack>

FillState::FillState(const QColor& fillColor)
    : m_fillColor(fillColor)
{
    // 构造函数简化，只初始化填充颜色
}

void FillState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    m_isPressed = true;
    
    // 设置等待光标，表示填充操作正在进行
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    // 执行填充操作
    fillRegion(drawArea, scenePos);
    
    // 重置光标
    QApplication::restoreOverrideCursor();
}

void FillState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    // 右键点击切换回编辑模式
    if (drawArea) {
        drawArea->setEditState();
        qDebug() << "填充工具: 右键点击，切换回编辑模式";
    }
}

void FillState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 获取鼠标点击位置
        QPointF scenePos = drawArea->mapToScene(event->pos());
        handleLeftMousePress(drawArea, scenePos);
        
        // 事件已处理
        event->accept();
    } else if (event->button() == Qt::RightButton) {
        QPointF scenePos = drawArea->mapToScene(event->pos());
        handleRightMousePress(drawArea, scenePos);
        
        // 事件已处理
        event->accept();
    }
}

void FillState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isPressed = false;
    }
}

void FillState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) {
    m_lastPoint = event->pos();
    
    // 更新状态信息
    QString statusMsg = QString("填充工具: 点击要填充的区域");
    QWidget* topLevelWidget = drawArea->window();
    if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
        mainWindow->statusBar()->showMessage(statusMsg);
    }
    
    // 确保使用填充工具的光标
    drawArea->setCursor(Qt::PointingHandCursor);
}

void FillState::paintEvent(DrawArea* drawArea, QPainter* painter) {
    // 无需额外绘制
}

void FillState::keyPressEvent(DrawArea* drawArea, QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        drawArea->setEditState();
    }
}

// 执行填充操作
void FillState::fillRegion(DrawArea* drawArea, const QPointF& startPoint) {
    // 获取场景矩形
    QRectF sceneRect = drawArea->scene()->sceneRect();
    
    qDebug() << "FillState: 执行填充 - 场景坐标:" << startPoint 
             << "填充颜色:" << m_fillColor.name(QColor::HexArgb);
    
    // 将整个场景渲染到图像中进行填充
    QImage image(qCeil(sceneRect.width()), qCeil(sceneRect.height()), QImage::Format_ARGB32);
    image.fill(Qt::transparent);  // 使用透明背景
    
    QPainter painter(&image);
    // 禁用抗锯齿，这样边界更明确
    painter.setRenderHint(QPainter::Antialiasing, false);
    
    // 调整坐标系，使场景左上角对应图像的(0,0)
    painter.translate(-sceneRect.topLeft());
    
    // 渲染场景到图像 - 确保渲染所有图层
    drawArea->scene()->render(&painter, sceneRect, sceneRect, Qt::KeepAspectRatio);
    painter.end();
    
    // 将场景坐标转换为图像坐标
    QPoint imagePoint(
        static_cast<int>(startPoint.x() - sceneRect.left()),
        static_cast<int>(startPoint.y() - sceneRect.top())
    );
    
    qDebug() << "FillState: 图像坐标" << imagePoint 
             << "图像大小:" << image.width() << "x" << image.height();
    
    // 检查图像坐标是否在有效范围内
    if (imagePoint.x() < 0 || imagePoint.y() < 0 || 
        imagePoint.x() >= image.width() || imagePoint.y() >= image.height()) {
        qDebug() << "FillState: 填充点不在有效图像范围内";
        
        // 显示错误消息
        QWidget* topLevelWidget = drawArea->window();
        if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
            mainWindow->statusBar()->showMessage("错误: 填充点不在有效区域内");
        }
        return;
    }
    
    // 获取目标颜色（种子点的颜色）
    QColor targetColor = image.pixelColor(imagePoint);
    qDebug() << "FillState: 目标颜色" << targetColor.name(QColor::HexArgb)
             << "RGBA(" << targetColor.red() << "," << targetColor.green()
             << "," << targetColor.blue() << "," << targetColor.alpha() << ")";
    
    // 如果目标颜色与填充颜色相同，则无需填充
    if (targetColor == m_fillColor) {
        qDebug() << "FillState: 目标颜色与填充颜色相同，无需填充";
        
        // 显示消息
        QWidget* topLevelWidget = drawArea->window();
        if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
            mainWindow->statusBar()->showMessage("提示: 目标区域已经是所选颜色");
        }
        return;
    }
    
    // 创建一个副本用于填充，保留原始图像
    QImage fillImage = image;
    
    // 直接执行填充操作
    int filledPixelsCount = fillImageRegion(fillImage, imagePoint, targetColor, m_fillColor);
    
    // 如果填充像素数太多（超过图像面积的30%），可能是误操作
    double fillRatio = static_cast<double>(filledPixelsCount) / (image.width() * image.height());
    qDebug() << "FillState: 填充比例:" << (fillRatio * 100) << "%";
    
    if (fillRatio > 0.3) {
        qDebug() << "FillState: 填充区域过大，可能是误操作";
        QWidget* topLevelWidget = drawArea->window();
        if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
            mainWindow->statusBar()->showMessage("警告: 填充区域过大，可能不是您想要的结果");
        }
    }
    
    if (filledPixelsCount > 0) {
        // 创建新的图层，仅包含填充的像素
        QImage resultImage(image.size(), QImage::Format_ARGB32);
        resultImage.fill(Qt::transparent);
        
        // 只复制被填充的像素
        for (int y = 0; y < image.height(); y++) {
            for (int x = 0; x < image.width(); x++) {
                if (fillImage.pixelColor(x, y) == m_fillColor &&
                    image.pixelColor(x, y) != m_fillColor) {
                    resultImage.setPixelColor(x, y, m_fillColor);
                }
            }
        }
        
        // 创建填充结果的图像项
        QPixmap fillResult = QPixmap::fromImage(resultImage);
        QGraphicsPixmapItem* fillItem = new QGraphicsPixmapItem(fillResult);
        fillItem->setPos(sceneRect.topLeft());  // 与场景左上角对齐
        fillItem->setZValue(-1);  // 降低Z轴值，确保在所有绘制内容下方
        drawArea->scene()->addItem(fillItem);
        
        // 显示成功消息
        QWidget* topLevelWidget = drawArea->window();
        if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
            mainWindow->statusBar()->showMessage(QString("填充完成: 已填充 %1 个像素").arg(filledPixelsCount));
        }
    } else {
        // 显示失败消息
        QWidget* topLevelWidget = drawArea->window();
        if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
            mainWindow->statusBar()->showMessage("填充失败: 未找到可填充的区域");
        }
    }
}

// 使用扫描线算法填充图像区域，返回填充的像素数
int FillState::fillImageRegion(QImage& image, const QPoint& seedPoint, 
                             const QColor& targetColor, const QColor& fillColor) {
    if (image.isNull() || !image.rect().contains(seedPoint)) {
        qDebug() << "FillState: 填充点不在有效图像范围内";
        return 0;
    }
    
    // 如果目标颜色和填充颜色相同，则无需填充
    if (targetColor == fillColor) {
        qDebug() << "FillState: 目标颜色与填充颜色相同，无需填充";
        return 0;
    }
    
    // 确保种子点的颜色与目标颜色匹配
    QColor seedColor = image.pixelColor(seedPoint);
    if (seedColor != targetColor) {
        qDebug() << "FillState: 种子点颜色与目标颜色不匹配";
        return 0;
    }
    
    // 使用扫描线算法优化填充
    QStack<QPoint> stack;
    stack.push(seedPoint);
    
    int width = image.width();
    int height = image.height();
    QVector<QVector<bool>> visited(width, QVector<bool>(height, false));
    
    int filledPixels = 0;
    
    // 记录填充区域的边界
    int minX = width, maxX = 0, minY = height, maxY = 0;
    
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
        
        // 更新边界范围
        minX = qMin(minX, left);
        maxX = qMax(maxX, right);
        minY = qMin(minY, y);
        maxY = qMax(maxY, y);
        
        // 填充当前行
        bool spanAbove = false;
        bool spanBelow = false;
        
        for (int i = left; i <= right; i++) {
            visited[i][y] = true;
            image.setPixelColor(i, y, fillColor);
            filledPixels++;
            
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
    
    if (filledPixels > 0) {
        // 记录填充区域大小
        int areaWidth = maxX - minX + 1;
        int areaHeight = maxY - minY + 1;
        qDebug() << "FillState: 填充区域大小:" << areaWidth << "x" << areaHeight;
    }
    
    qDebug() << "FillState: 填充完成 - 已填充" << filledPixels << "个像素";
    return filledPixels;
}
