#ifndef FILE_FORMAT_MANAGER_H
#define FILE_FORMAT_MANAGER_H

#include <QString>
#include <QGraphicsScene>
#include <QList>
#include <QGraphicsItem>
#include <QDataStream>
#include <functional>
#include <memory>
#include "../core/graphic.h"

class QIODevice;
class GraphicItem;
class ConnectionManager;
class ConnectionPointOverlay;
class SelectionManager;

/**
 * @brief 文件格式管理器 - 用于处理自定义矢量文件格式和SVG导出
 */
class FileFormatManager {
public:
    // 单例实例访问
    static FileFormatManager& getInstance();

    // 禁止拷贝和赋值
    FileFormatManager(const FileFormatManager&) = delete;
    FileFormatManager& operator=(const FileFormatManager&) = delete;

    // 自定义矢量格式 (.cvg) 操作
    bool saveToCustomFormat(const QString& filePath, QGraphicsScene* scene);
    bool loadFromCustomFormat(const QString& filePath, QGraphicsScene* scene, 
                              std::function<GraphicItem*(Graphic::GraphicType, const QPointF&, const QPen&, const QBrush&, 
                                                 const std::vector<QPointF>&, double, const QPointF&)> itemFactory,
                              ConnectionManager* connectionManager = nullptr,
                              ConnectionPointOverlay* connectionOverlay = nullptr,
                              SelectionManager* selectionManager = nullptr);

    // SVG导出
    bool exportToSVG(const QString& filePath, QGraphicsScene* scene, const QSize& size = QSize());

    // 文件格式常量
    static const QString CVG_EXTENSION;  // 自定义矢量格式扩展名
    static const QString CVG_MIME_TYPE;  // 自定义矢量格式MIME类型
    static constexpr qint32 CVG_VERSION = 1;  // 当前文件格式版本

private:
    FileFormatManager() = default;
    ~FileFormatManager() = default;

    // 序列化图形项辅助方法
    bool serializeGraphicItems(QDataStream& stream, const QList<QGraphicsItem*>& items);
    bool deserializeGraphicItems(QDataStream& stream, QGraphicsScene* scene,
                                std::function<GraphicItem*(Graphic::GraphicType, const QPointF&, const QPen&, const QBrush&, 
                                                 const std::vector<QPointF>&, double, const QPointF&)> itemFactory,
                                ConnectionManager* connectionManager = nullptr,
                                ConnectionPointOverlay* connectionOverlay = nullptr,
                                SelectionManager* selectionManager = nullptr);

    // 图层信息序列化辅助方法
    bool serializeLayers(QDataStream& stream, QGraphicsScene* scene);
    bool deserializeLayers(QDataStream& stream, QGraphicsScene* scene);

    // SVG转换辅助方法
    bool graphicItemToSVG(QIODevice* device, const QList<QGraphicsItem*>& items, const QSize& size);
};

#endif // FILE_FORMAT_MANAGER_H 