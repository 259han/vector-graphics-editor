#ifndef DRAW_AREA_H
#define DRAW_AREA_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPainter>
#include <memory>
#include "../core/graphics_item_factory.h"
#include "../core/selection_manager.h"
#include "../core/connection_manager.h"
#include "../core/connection_point_overlay.h"
#include "../state/editor_state.h"

#include "image_resizer.h"
#include "../core/flowchart_connector_item.h"

class GraphicItem;
class ImageResizer;

class DrawArea : public QGraphicsView {
    Q_OBJECT

public:
    explicit DrawArea(QWidget *parent = nullptr);
    ~DrawArea() override;

    // 场景访问方法
    QGraphicsScene* scene() const { return m_scene; }
    
    // 性能优化相关方法
    void setHighQualityRendering(bool enable);
    bool isHighQualityRendering() const { return m_highQualityRendering; }
    
    // 工厂访问
    DefaultGraphicsItemFactory* getGraphicFactory();
    
    // 状态管理
    EditorState* getCurrentState() const { return m_currentState.get(); }
    void setDrawState(GraphicItem::GraphicType type);
    void setEditState();
    void setFillState();
    void setFillState(const QColor& color);
    void setClipState();
    void setClipState(bool freehandMode);
    void setAutoConnectState();

    // 填充相关
    void setFillColor(const QColor& color);
    QColor getFillColor() const { return m_fillColor; }
    void setLineColor(const QColor& color);
    QColor getLineColor() const { return m_lineColor; }
    void setLineWidth(int width);
    int getLineWidth() const { return m_lineWidth; }
    
    // 流程图连接器样式设置
    void setConnectorType(FlowchartConnectorItem::ConnectorType type);
    FlowchartConnectorItem::ConnectorType getConnectorType() const { return m_connectorType; }
    void setArrowType(FlowchartConnectorItem::ArrowType type);
    FlowchartConnectorItem::ArrowType getArrowType() const { return m_arrowType; }

    // 图像操作
    void setImage(const QImage& image);
    void clearGraphics();
    void saveImage();
    void importImage();
    void importImageAt(const QImage &image, const QPoint &pos);

    // 新增文件格式支持方法
    bool saveToCustomFormat(const QString& filePath); // 保存为.cvg格式
    bool loadFromCustomFormat(const QString& filePath); // 从.cvg格式加载
    bool exportToSVG(const QString& filePath, const QSize& size = QSize()); // 导出为SVG
    void saveAsWithFormatDialog(); // 带格式选择的保存对话框
    void openWithFormatDialog(); // 带格式选择的打开对话框
    
    // 性能优化相关方法
    void saveImageOptimized();
    void saveImageWithOptions();
    void exportLargeImage(const QString& filePath, const QSize& size, bool transparent = false);

    // 视口交互功能
    void enableGrid(bool enable);
    void setGridSize(int size);
    bool isGridEnabled() const { return m_gridEnabled; }
    int getGridSize() const { return m_gridSize; }
    
    // 选中图形操作 - 重构为使用SelectionManager
    void moveSelectedGraphics(const QPointF& offset);
    void rotateSelectedGraphics(double angle);
    void scaleSelectedGraphics(double factor);
    void flipSelectedGraphics(bool horizontal);
    void deleteSelectedGraphics();
    
    // 选择所有图形
    void selectAllGraphics();
    
    // 图层操作
    void bringToFront(QGraphicsItem* item);
    void sendToBack(QGraphicsItem* item);
    void bringForward(QGraphicsItem* item);
    void sendBackward(QGraphicsItem* item);
    
    // 获取选中的图形项 - 使用SelectionManager
    QList<QGraphicsItem*> getSelectedItems() const;
    
    // 增强的剪贴板操作
    void copySelectedItems();
    void pasteItems();
    void pasteItemsAtPosition(const QPointF& pos);
    void cutSelectedItems();
    
    // 剪贴板交互
    void copyToSystemClipboard();
    void pasteFromSystemClipboard();
    bool canPasteFromClipboard() const;

    // 添加图像调整器
    void addImageResizer(ImageResizer* resizer);
    
    // 获取选择管理器
    SelectionManager* getSelectionManager() const { return m_selectionManager.get(); }
    
    // 获取连接管理器
    ConnectionManager* getConnectionManager() const { return m_connectionManager.get(); }
    ConnectionPointOverlay* getConnectionOverlay() const { return m_connectionOverlay; }
    
    // 处理新创建的图形项（包括自动注册流程图元素）
    void handleNewGraphicItem(QGraphicsItem* item);
    
    // 更新连接管理器
    void updateConnectionManager();
    
    // 设置选择模式
    void setSelectionMode(SelectionManager::SelectionMode mode);
    
    // 设置选择过滤器
    void setSelectionFilter(const SelectionManager::SelectionFilter& filter);
    void clearSelectionFilter();
    
    // 清除当前选择
    void clearSelection();

    // 缓存控制方法
    void enableGraphicsCaching(bool enable);
    bool isGraphicsCachingEnabled() const { return m_graphicsCachingEnabled; }
    
    // 裁剪优化方法
    void enableClippingOptimization(bool enable);
    bool isClippingOptimizationEnabled() const { return m_clippingOptimizationEnabled; }

signals:
    // 选择变更信号
    void selectionChanged();
    
    // 性能相关信号
    void cachingStatusChanged(bool enabled);
    void clippingStatusChanged(bool enabled);
    
    // 状态消息信号
    void statusMessageChanged(const QString& message, int timeout);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QGraphicsScene* m_scene;
    std::unique_ptr<DefaultGraphicsItemFactory> m_graphicFactory;
    std::unique_ptr<EditorState> m_currentState;
    std::unique_ptr<SelectionManager> m_selectionManager;
    std::unique_ptr<ConnectionManager> m_connectionManager;
    ConnectionPointOverlay* m_connectionOverlay;
    
    // 更新控制
    bool m_updatePending = false;
    void scheduleUpdate();
    
    // 渲染质量控制
    bool m_highQualityRendering = true;
    
    // 平移和缩放属性
    bool m_spaceKeyPressed = false;
    bool m_isPanning = false;
    QPointF m_lastPanPoint;
    
    // 网格属性
    bool m_gridEnabled = false;
    int m_gridSize = 20;
    
    // 样式属性
    QColor m_fillColor = Qt::black;
    QColor m_lineColor = Qt::black;
    int m_lineWidth = 2;
    
    // 流程图连接器样式
    FlowchartConnectorItem::ConnectorType m_connectorType = FlowchartConnectorItem::StraightLine;
    FlowchartConnectorItem::ArrowType m_arrowType = FlowchartConnectorItem::SingleArrow;
    
    // 剪贴板相关
    // 内部剪贴板
    struct ClipboardItem {
        GraphicItem::GraphicType type;
        QPen pen;
        QBrush brush;
        std::vector<QPointF> points;
        QPointF position;
        double rotation = 0.0;
        QPointF scale = QPointF(1.0, 1.0);
    };
    QList<ClipboardItem> m_clipboardData;
    bool m_isClipboardFromCut = false;
    
    // 用于系统剪贴板的MIME类型
    static const QString MIME_GRAPHICITEMS;
    
    // 辅助方法
    QGraphicsItem* createItemFromClipboardData(const ClipboardItem& data, const QPointF& pastePosition);
    QByteArray serializeGraphicItems(const QList<QGraphicsItem*>& items);
    QList<ClipboardItem> deserializeGraphicItems(const QByteArray& data);
    void createContextMenu(const QPoint& pos);
    QPointF calculateSmartPastePosition() const;
    QPointF getViewCenterScenePos() const;
    void saveGraphicItemToClipboard(GraphicItem* item);

    QList<ImageResizer*> m_imageResizers;

    // 设置为DrawArea类的友元，以便可以访问私有成员
    friend class FillState;
    
    // 控制键状态
    bool m_ctrlKeyPressed = false;

    // 缓存和裁剪优化属性
    bool m_graphicsCachingEnabled = false;
    bool m_clippingOptimizationEnabled = true;
    
    // 内部辅助方法
    void updateGraphicsCaching();
    void optimizeVisibleItems();
    QImage renderSceneToImage(const QRectF& sceneRect, bool transparent = false);
    void renderScenePart(QPainter* painter, const QRectF& targetRect, const QRectF& sourceRect);
    
    // 分块渲染大图像的辅助方法
    bool exportLargeImageTiled(const QString& filePath, const QSize& size, bool transparent = false);
};

#endif // DRAW_AREA_H