#include "../../include/ui/draw_area.h"
#include "../../include/state/draw_state.h"
#include "../../include/state/edit_state.h"
#include <QPaintEvent>
#include <QMouseEvent> 
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QPainter>
#include <QDebug>

DrawArea::DrawArea(QWidget *parent) : QWidget(parent) {
    m_graphicFactory = std::make_unique<DefaultGraphicFactory>();
    setEditState();  
    setMinimumSize(800, 600);
    setMouseTracking(true);
    // 初始化空白图像
    m_image = QImage(size(), QImage::Format_ARGB32);
    m_image.fill(Qt::white);
}
void DrawArea::setImage(const QImage& image) {
    m_image = image; // 确保图像被正确设置
    update(); // 更新绘图区域
}

GraphicManager& DrawArea::getGraphicManager() {
    return m_graphicManager;
}

GraphicFactory* DrawArea::getGraphicFactory() {
    return m_graphicFactory.get();
}

void DrawArea::setDrawState(Graphic::GraphicType type) {
    m_currentState = std::make_unique<DrawState>(type);
}

void DrawArea::setEditState() {
    m_currentState = std::make_unique<EditState>();
}

void DrawArea::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);

    // 直接使用 m_image，不重置
    if (m_image.isNull()) {
        m_image = QImage(size(), QImage::Format_ARGB32);
        m_image.fill(Qt::white);
    }

    // 创建临时图像用于绘制图形
    QImage tempImage = m_image.copy();

    QPainter imagePainter(&tempImage);
    for (const auto& graphic : m_graphicManager.getGraphics()) {
        graphic->draw(imagePainter);
    }

    // 绘制背景图像
    painter.drawImage(0, 0, tempImage);

    // 绘制当前状态的临时内容（如预览图形）
    if (m_currentState) {
        m_currentState->paintEvent(this, &painter);
    }

    // 更新 m_image 以保留已提交的图形
    m_image = tempImage;
}


void DrawArea::mousePressEvent(QMouseEvent *event) {
    if (m_currentState) {
        m_currentState->mousePressEvent(this, event);
    }
}

void DrawArea::mouseReleaseEvent(QMouseEvent *event) {
    if (m_currentState) {
        m_currentState->mouseReleaseEvent(this, event);
    }
}

void DrawArea::mouseMoveEvent(QMouseEvent *event) {
    if (m_currentState) {
        m_currentState->mouseMoveEvent(this, event);
    }
}

void DrawArea::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    if (!m_image.isNull()) {
        // 创建新图像并缩放原始图像
        QImage newImage(size(), QImage::Format_ARGB32);
        newImage.fill(Qt::white);
        QPainter painter(&newImage);
        painter.drawImage(0, 0, m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_image = newImage;
    } else {
        m_image = QImage(size(), QImage::Format_ARGB32);
        m_image.fill(Qt::white);
    }
}

EditorState* DrawArea::getCurrentDrawState() {
    return m_currentState.get(); // 返回当前状态的指针
}

void DrawArea::clearGraphics() {
    m_graphicManager.clearGraphics(); // 清空所有图形对象
    m_image.fill(Qt::white);          // 将图像重置为白色

    // 检查当前状态是否为 DrawState 类型
    if (auto* drawState = dynamic_cast<DrawState*>(m_currentState.get())) {
        drawState->resetFillMode(); // 重置填色模式
    }

    update();                         // 更新绘图区域
}

void DrawArea::importImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)");
    if (fileName.isEmpty()) return;

    QImage image(fileName);
    if (image.isNull()) {
        QMessageBox::warning(this, "Import Image", "Failed to load image.");
        return;
    }

    // 缩放图片以完全匹配绘图区域的尺寸
    image = image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 创建与绘图区域大小一致的图像
    m_image = QImage(size(), QImage::Format_ARGB32);
    m_image.fill(Qt::white); // 设置默认背景为白色

    // 将缩放后的图片绘制到 m_image 上
    QPainter painter(&m_image);
    painter.drawImage(0, 0, image);

    update(); // 更新绘图区域
}

void DrawArea::saveImage() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)");
    if (fileName.isEmpty()) return;

    if (!m_image.save(fileName)) {
        QMessageBox::warning(this, "Save Image", "Failed to save image.");
    }
}