#ifndef DRAW_AREA_H
#define DRAW_AREA_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPainter>
#include <memory>
#include "../core/graphics_item_factory.h"
#include "../state/editor_state.h"
#include "image_resizer.h"

class GraphicItem;
class ImageResizer;
class ImageManager;

class DrawArea : public QGraphicsView {
    Q_OBJECT

public:
    explicit DrawArea(QWidget *parent = nullptr);
    ~DrawArea() override;

    // 场景访问方法
    QGraphicsScene* scene() const { return m_scene; }
    
    // 工厂访问
    DefaultGraphicsItemFactory* getGraphicFactory();
    
    // 状态管理
    EditorState* getCurrentDrawState();
    void setDrawState(Graphic::GraphicType type);
    void setEditState();
    void setFillState();
    void setFillState(const QColor& color);

    // 填充相关
    void setFillColor(const QColor& color);
    QColor getFillColor() const { return m_fillColor; }

    // 图像操作
    void setImage(const QImage& image);
    void clearGraphics();
    void saveImage();
    void importImage();

    // 视口交互功能
    void enableGrid(bool enable);
    void setGridSize(int size);
    
    // 选中图形操作
    void moveSelectedGraphics(const QPointF& offset);
    void rotateSelectedGraphics(double angle);
    void scaleSelectedGraphics(double factor);
    void deleteSelectedGraphics();
    
    // 选择所有图形
    void selectAllGraphics();
    
    // 图层操作
    void bringToFront(QGraphicsItem* item);
    void sendToBack(QGraphicsItem* item);
    void bringForward(QGraphicsItem* item);
    void sendBackward(QGraphicsItem* item);
    
    // 获取选中的图形项
    QList<QGraphicsItem*> getSelectedItems() const;
    
    // 复制、粘贴操作
    void copySelectedItems();
    void pasteItems();
    void cutSelectedItems();

    // 添加图像调整器
    void addImageResizer(ImageResizer* resizer);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    QGraphicsScene* m_scene;
    std::unique_ptr<DefaultGraphicsItemFactory> m_graphicFactory;
    std::unique_ptr<EditorState> m_currentState;
    
    // 平移和缩放属性
    bool m_spaceKeyPressed = false;
    bool m_isPanning = false;
    QPointF m_lastPanPoint;
    
    // 网格属性
    bool m_gridEnabled = false;
    int m_gridSize = 20;
    
    // 填充属性
    QColor m_fillColor = Qt::black;
    
    // 复制缓冲区
    QList<QGraphicsItem*> m_copyBuffer;

    QList<ImageResizer*> m_imageResizers;
    ImageManager* m_imageManager;

    // 设置为DrawArea类的友元，以便可以访问私有成员
    friend class FillState;
};

#endif // DRAW_AREA_H