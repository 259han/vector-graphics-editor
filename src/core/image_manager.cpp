#include "core/image_manager.h"
#include "ui/draw_area.h"
#include "ui/image_resizer.h"

#include <QGraphicsScene>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QImageWriter>
#include <QPixmap>
#include <QDateTime>
#include <QFileInfo>
#include <QInputDialog>
#include <QDebug>
#include <QTimer>

ImageManager::ImageManager(QObject* parent)
    : QObject(parent)
    , m_drawArea(nullptr)
{
}

ImageManager::~ImageManager()
{
}

void ImageManager::setDrawArea(DrawArea* drawArea)
{
    m_drawArea = drawArea;
}

bool ImageManager::importImage(const QString& fileName)
{
    if (!m_drawArea) {
        qWarning() << "ImageManager::importImage: DrawArea is not set";
        return false;
    }

    QImage image;
    
    // 如果文件存在且可读
    if (QFile::exists(fileName) && QFileInfo(fileName).isReadable()) {
        // 使用QImageReader以获得更多控制和更好的错误处理
        QImageReader reader(fileName);
        
        // 为大图像设置更大的内存限制
        QImageReader::setAllocationLimit(512); // 默认值是256MB
        
        // 获取图像尺寸
        QSize imageSize = reader.size();
        if (!imageSize.isValid()) {
            QMessageBox::critical(
                m_drawArea,
                tr("导入错误"),
                tr("无法读取图像文件:\n%1\n\n错误: %2")
                    .arg(fileName)
                    .arg(reader.errorString())
            );
            qDebug() << "Cannot read image file:" << fileName << "Error:" << reader.errorString();
            return false;
        }
        
        // 如果图像过大，直接设置缩放
        const int MAX_SIZE = 1500;
        if (imageSize.width() > MAX_SIZE || imageSize.height() > MAX_SIZE) {
            QSize newSize = imageSize;
            if (imageSize.width() > imageSize.height()) {
                newSize.setWidth(MAX_SIZE);
                newSize.setHeight(imageSize.height() * MAX_SIZE / imageSize.width());
            } else {
                newSize.setHeight(MAX_SIZE);
                newSize.setWidth(imageSize.width() * MAX_SIZE / imageSize.height());
            }
            
            reader.setScaledSize(newSize);
            qDebug() << "Resizing image from" << imageSize << "to" << newSize;
        }
        
        // 读取图像
        image = reader.read();
        
        if (image.isNull()) {
            QMessageBox::critical(
                m_drawArea,
                tr("导入错误"),
                tr("无法加载图像:\n%1\n\n错误: %2")
                    .arg(fileName)
                    .arg(reader.errorString())
            );
            qDebug() << "Failed to load image:" << fileName << "Error:" << reader.errorString();
            return false;
        }
    } else {
        QMessageBox::critical(
            m_drawArea,
            tr("导入错误"),
            tr("文件不存在或不可读:\n%1").arg(fileName)
        );
        qDebug() << "File does not exist or is not readable:" << fileName;
        return false;
    }
    
    // 添加图像到场景
    QGraphicsPixmapItem* item = addImageToScene(image);
    if (!item) {
        return false;
    }
    
    // 提示成功
    QMessageBox::information(
        m_drawArea,
        tr("导入成功"),
        tr("成功导入图像:\n%1\n尺寸: %2x%3")
            .arg(fileName)
            .arg(image.width())
            .arg(image.height())
    );
    
    qDebug() << "Successfully imported image:" << fileName 
             << "Size:" << image.width() << "x" << image.height();
    
    return true;
}

QGraphicsPixmapItem* ImageManager::addImageToScene(const QImage& image)
{
    if (!m_drawArea) {
        qWarning() << "ImageManager::addImageToScene: DrawArea is not set";
        return nullptr;
    }
    
    if (image.isNull()) {
        qWarning() << "ImageManager::addImageToScene: Attempted to add null image";
        return nullptr;
    }
    
    // 将图片转换为pixmap
    QPixmap pixmap = QPixmap::fromImage(image);
    
    if (pixmap.isNull()) {
        qWarning() << "ImageManager::addImageToScene: Failed to convert image to pixmap";
        return nullptr;
    }
    
    QGraphicsScene* scene = m_drawArea->scene();
    if (!scene) {
        qWarning() << "ImageManager::addImageToScene: Scene is not available";
        return nullptr;
    }
    
    // 清除当前场景的选择（不是清除整个场景）
    scene->clearSelection();
    
    // 创建QGraphicsPixmapItem
    QGraphicsPixmapItem* pixmapItem = new QGraphicsPixmapItem();
    pixmapItem->setPixmap(pixmap);
    
    // 设置图片属性 - 确保设置所有必要的交互标志
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsFocusable, true);
    
    // 设置变换原点为图像中心 - 对于旋转很重要
    pixmapItem->setTransformOriginPoint(pixmapItem->boundingRect().center());
    
    // 启用接受悬停事件以提高交互性
    pixmapItem->setAcceptHoverEvents(true);
    
    // 设置图片位置为场景中心
    QRectF sceneRect = scene->sceneRect();
    QPointF center = sceneRect.center();
    
    // 添加到场景中
    scene->addItem(pixmapItem);
    
    // 计算居中位置并移动
    pixmapItem->setPos(center.x() - pixmap.width()/2, center.y() - pixmap.height()/2);
    
    // 将视图居中到图片
    m_drawArea->centerOn(pixmapItem);
    
    // 选中图片并设置焦点
    pixmapItem->setSelected(true);
    pixmapItem->setFocus();
    
    // 切换到编辑状态，确保能立即操作图片
    m_drawArea->setEditState();
    
    // 延迟创建ImageResizer，确保pixmapItem已完全添加到场景
    QTimer::singleShot(100, [this, pixmapItem]() {
        if (pixmapItem && pixmapItem->scene()) {
            // 创建调整大小控制器
            ImageResizer* resizer = new ImageResizer(pixmapItem);
            
            // 确保调整器的可见性
            resizer->setVisible(true);
            
            // 强制更新控制点位置
            resizer->updateHandles();
            
            // 记录这个调整器，以便后续管理
            m_drawArea->addImageResizer(resizer);
            
            qDebug() << "ImageResizer created with delayed initialization";
        } else {
            qWarning() << "Failed to create ImageResizer - pixmapItem not in scene after delay";
        }
    });
    
    qDebug() << "Image added to scene with size:" << pixmap.width() << "x" << pixmap.height();
    qDebug() << "Image item flags:" << pixmapItem->flags();
    
    return pixmapItem;
}

bool ImageManager::saveImage(const QString& fileName, const QString& format, int quality)
{
    if (!m_drawArea) {
        qWarning() << "ImageManager::saveImage: DrawArea is not set";
        return false;
    }

    QGraphicsScene* scene = m_drawArea->scene();
    if (!scene) {
        qWarning() << "ImageManager::saveImage: Scene is not available";
        return false;
    }
    
    QString actualFileName = fileName;
    QString actualFormat = format;
    int actualQuality = quality;
    
    // 如果没有提供文件名，弹出保存对话框
    if (actualFileName.isEmpty()) {
        // 获取默认文件名（日期时间）
        QString defaultFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
        
        // 定义支持的文件格式
        QString filters = tr("PNG图像 (*.png);;JPEG图像 (*.jpg *.jpeg);;BMP图像 (*.bmp);;所有文件 (*)");
        
        // 保存场景为图像
        QString selectedFilter;
        actualFileName = QFileDialog::getSaveFileName(
            m_drawArea, 
            tr("保存图像"), 
            defaultFileName, 
            filters,
            &selectedFilter
        );
        
        if (actualFileName.isEmpty()) {
            return false; // 用户取消了操作
        }
        
        // 根据选择的过滤器确定文件格式
        if (selectedFilter.contains("PNG")) {
            actualFormat = "PNG";
            // 确保文件名有.png扩展名
            if (!actualFileName.endsWith(".png", Qt::CaseInsensitive)) {
                actualFileName += ".png";
            }
        } else if (selectedFilter.contains("JPEG")) {
            actualFormat = "JPEG";
            actualQuality = 90; // JPEG默认质量
            // 确保文件名有.jpg扩展名
            if (!actualFileName.endsWith(".jpg", Qt::CaseInsensitive) && 
                !actualFileName.endsWith(".jpeg", Qt::CaseInsensitive)) {
                actualFileName += ".jpg";
            }
        } else if (selectedFilter.contains("BMP")) {
            actualFormat = "BMP";
            // 确保文件名有.bmp扩展名
            if (!actualFileName.endsWith(".bmp", Qt::CaseInsensitive)) {
                actualFileName += ".bmp";
            }
        } else {
            // 尝试从文件扩展名推断格式
            QFileInfo info(actualFileName);
            QString suffix = info.suffix().toLower();
            
            if (suffix == "png") {
                actualFormat = "PNG";
            } else if (suffix == "jpg" || suffix == "jpeg") {
                actualFormat = "JPEG";
                actualQuality = 90;
            } else if (suffix == "bmp") {
                actualFormat = "BMP";
            } else {
                // 默认使用PNG
                actualFormat = "PNG";
                if (suffix.isEmpty()) {
                    actualFileName += ".png";
                }
            }
        }
    }
    
    // 对于JPEG格式，让用户选择压缩质量
    if (actualFormat == "JPEG" && actualQuality > 0) {
        bool ok;
        actualQuality = QInputDialog::getInt(
            m_drawArea, 
            tr("JPEG质量"), 
            tr("设置JPEG图像质量 (1-100):"), 
            actualQuality, 1, 100, 1, 
            &ok
        );
        
        if (!ok) {
            actualQuality = 90; // 如果用户取消，使用默认质量
        }
    }
    
    // 捕获可能的错误
    try {
        // 计算实际内容的边界, 而不是使用整个场景矩形
        QRectF contentRect;
        
        // 如果场景中有图形项，则计算包含所有图形项的边界矩形
        if (!scene->items().isEmpty()) {
            contentRect = scene->itemsBoundingRect();
            
            // 添加一点边距
            qreal margin = 20;
            contentRect.adjust(-margin, -margin, margin, margin);
        } else {
            // 如果场景为空，使用默认大小
            contentRect = QRectF(-400, -300, 800, 600);
        }
        
        // 确保矩形有最小大小
        if (contentRect.width() < 100) contentRect.setWidth(100);
        if (contentRect.height() < 100) contentRect.setHeight(100);
        
        qDebug() << "Content bounding rect for save:" << contentRect;
        
        // 图像尺寸
        int width = qRound(contentRect.width());
        int height = qRound(contentRect.height());
        
        // 如果尺寸过大，按比例缩小到合理大小
        const int MAX_SIZE = 3000;
        if (width > MAX_SIZE || height > MAX_SIZE) {
            if (width > height) {
                height = height * MAX_SIZE / width;
                width = MAX_SIZE;
            } else {
                width = width * MAX_SIZE / height;
                height = MAX_SIZE;
            }
            
            qDebug() << "Resized output image to:" << width << "x" << height;
        }
        
        // 创建透明背景的图像
        QImage image(width, height, QImage::Format_ARGB32);
        image.fill(Qt::transparent); // 使用透明背景
        
        // 使用QPainter渲染场景到图像
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        
        // 设置变换以确保场景内容完全适应图像
        painter.setViewport(0, 0, width, height);
        painter.setWindow(contentRect.toRect());
        
        // 禁用背景以使用透明背景
        QBrush oldBrush = scene->backgroundBrush();
        scene->setBackgroundBrush(Qt::NoBrush);
        
        // 渲染场景到图像
        scene->render(&painter, QRectF(), contentRect);
        
        // 恢复原始背景
        scene->setBackgroundBrush(oldBrush);
        
        // 结束绘制
        painter.end();
        
        // 保存图像
        QImageWriter writer(actualFileName, actualFormat.toUtf8());
        
        // 设置质量（如果是JPEG）
        if (actualFormat == "JPEG" && actualQuality > 0) {
            writer.setQuality(actualQuality);
        }
        
        // 写入图像
        if (!writer.write(image)) {
            QMessageBox::critical(
                m_drawArea, 
                tr("保存错误"), 
                tr("保存图像失败:\n%1\n\n错误: %2")
                    .arg(actualFileName)
                    .arg(writer.errorString())
            );
            qWarning() << "Failed to save image:" << actualFileName << "Error:" << writer.errorString();
            return false;
        }
        
        // 显示成功消息
        QMessageBox::information(
            m_drawArea, 
            tr("保存成功"), 
            tr("成功保存图像:\n%1\n尺寸: %2x%3")
                .arg(actualFileName)
                .arg(width)
                .arg(height)
        );
        
        qDebug() << "Successfully saved image:" << actualFileName 
                 << "Size:" << width << "x" << height
                 << "Format:" << actualFormat
                 << (actualFormat == "JPEG" ? QString("Quality: %1").arg(actualQuality) : QString());
                 
        return true;
    } catch (const std::exception& e) {
        QMessageBox::critical(
            m_drawArea,
            tr("保存错误"),
            tr("保存图像时发生异常:\n%1").arg(e.what())
        );
        qWarning() << "Exception during image save:" << e.what();
        return false;
    } catch (...) {
        QMessageBox::critical(
            m_drawArea,
            tr("保存错误"),
            tr("保存图像时发生未知错误")
        );
        qWarning() << "Unknown exception during image save";
        return false;
    }
} 