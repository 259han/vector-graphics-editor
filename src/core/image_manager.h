#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

class DrawArea;

class ImageManager : public QObject
{
    Q_OBJECT

public:
    explicit ImageManager(QObject* parent = nullptr);
    ~ImageManager();

    // 设置关联的绘图区域
    void setDrawArea(DrawArea* drawArea);

    // 导入图像
    bool importImage(const QString& fileName);
    
    // 保存图像
    bool saveImage(const QString& fileName, const QString& format = QString(), int quality = -1);

    // 将图像添加到场景
    QGraphicsPixmapItem* addImageToScene(const QImage& image);

private:
    DrawArea* m_drawArea;
};

#endif // IMAGE_MANAGER_H 