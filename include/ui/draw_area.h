#ifndef DRAW_AREA_H
#define DRAW_AREA_H

#include <QWidget>
#include <QPainter>
#include <memory>
#include "../core/graphic_manager.h"
#include "../core/graphic_factory.h"
#include "../state/editor_state.h"

class DrawArea : public QWidget {
    Q_OBJECT

public:
    explicit DrawArea(QWidget *parent = nullptr);

    GraphicManager& getGraphicManager();
    GraphicFactory* getGraphicFactory();
    EditorState* getCurrentDrawState();

    void setDrawState(Graphic::GraphicType type);
    void setEditState();

    void setImage(const QImage& image);

    void clearGraphics();

    QImage m_image;

    void saveImage();
    void importImage();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    GraphicManager m_graphicManager;
    std::unique_ptr<GraphicFactory> m_graphicFactory;
    std::unique_ptr<EditorState> m_currentState;
};

#endif // DRAW_AREA_H