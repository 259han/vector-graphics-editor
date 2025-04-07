#include "graphics_utils.h"
#include "logger.h"
#include <QPainter>
#include <QRect>

QImage GraphicsUtils::renderSceneToImage(QGraphicsScene* scene, bool disableAntialiasing) {
    if (!scene) return QImage();
    
    // 获取场景矩形，确保至少是1x1大小
    QRectF sceneRect = scene->sceneRect();
    if (sceneRect.isEmpty()) {
        sceneRect = QRectF(0, 0, 1, 1);
    }
    
    // 创建透明背景的图像
    QImage image(sceneRect.width(), sceneRect.height(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    
    // 使用QPainter渲染场景
    QPainter painter(&image);
    
    // 设置渲染选项
    if (disableAntialiasing) {
        painter.setRenderHint(QPainter::Antialiasing, false);
    } else {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    }
    
    // 渲染场景
    scene->render(&painter, QRectF(), sceneRect);
    
    return image;
}

int GraphicsUtils::fillImageRegion(QImage& image, const QPoint& seedPoint, 
                                 const QColor& targetColor, const QColor& fillColor) {
    if (image.isNull() || !isPointInImageBounds(seedPoint, image.width(), image.height())) {
        qDebug() << "GraphicsUtils: 填充点不在有效图像范围内";
        return 0;
    }
    
    // 如果目标颜色和填充颜色相同，则无需填充
    if (targetColor == fillColor) {
        qDebug() << "GraphicsUtils: 目标颜色与填充颜色相同，无需填充";
        return 0;
    }
    
    // 确保种子点的颜色与目标颜色匹配
    QColor seedColor = image.pixelColor(seedPoint);
    if (seedColor != targetColor) {
        qDebug() << "GraphicsUtils: 种子点颜色与目标颜色不匹配";
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
        QPoint current = stack.pop();
        int x = current.x();
        int y = current.y();
        
        // 如果已访问或不是目标颜色，则跳过
        if (visited[x][y] || image.pixelColor(x, y) != targetColor) {
            continue;
        }
        
        // 查找当前扫描线的最左边界
        int left = x;
        while (left > 0 && image.pixelColor(left - 1, y) == targetColor && !visited[left - 1][y]) {
            left--;
        }
        
        // 查找当前扫描线的最右边界
        int right = x;
        while (right < width - 1 && image.pixelColor(right + 1, y) == targetColor && !visited[right + 1][y]) {
            right++;
        }
        
        // 更新边界
        minX = qMin(minX, left);
        maxX = qMax(maxX, right);
        minY = qMin(minY, y);
        maxY = qMax(maxY, y);
        
        // 填充当前扫描线
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
        logFillAreaStats(filledPixels, minX, minY, maxX, maxY);
    }
    
    qDebug() << "GraphicsUtils: 填充完成 - 已填充" << filledPixels << "个像素";
    return filledPixels;
}

double GraphicsUtils::calculateFillRatio(int filledPixels, int width, int height) {
    if (width <= 0 || height <= 0) return 0.0;
    return static_cast<double>(filledPixels) / (width * height);
}

QImage GraphicsUtils::createFillResultLayer(const QImage& originalImage, 
                                         const QImage& filledImage, 
                                         const QColor& fillColor) {
    if (originalImage.isNull() || filledImage.isNull() || 
        originalImage.size() != filledImage.size()) {
        return QImage();
    }
    
    QImage resultImage(originalImage.size(), QImage::Format_ARGB32);
    resultImage.fill(Qt::transparent);
    
    // 只复制被填充的像素
    for (int y = 0; y < originalImage.height(); y++) {
        for (int x = 0; x < originalImage.width(); x++) {
            if (filledImage.pixelColor(x, y) == fillColor &&
                originalImage.pixelColor(x, y) != fillColor) {
                resultImage.setPixelColor(x, y, fillColor);
            }
        }
    }
    
    return resultImage;
}

bool GraphicsUtils::isPointInImageBounds(const QPoint& point, int width, int height) {
    return point.x() >= 0 && point.y() >= 0 && point.x() < width && point.y() < height;
}

QPoint GraphicsUtils::sceneToImageCoordinates(const QPointF& scenePos, const QRectF& sceneRect) {
    return QPoint(
        static_cast<int>(scenePos.x() - sceneRect.left()),
        static_cast<int>(scenePos.y() - sceneRect.top())
    );
}

void GraphicsUtils::logFillAreaStats(int filledPixels, int minX, int minY, int maxX, int maxY) {
    if (filledPixels <= 0) return;
    
    int areaWidth = maxX - minX + 1;
    int areaHeight = maxY - minY + 1;
    
    // 使用新的Logger类记录日志，替代直接的qDebug
    Logger::debug(QString("GraphicsUtils: 填充区域大小: %1 x %2, 填充了 %3 个像素")
                 .arg(areaWidth).arg(areaHeight).arg(filledPixels));
} 