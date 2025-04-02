#include "../../include/ui/draw_area.h"
#include "../../include/state/draw_state.h"
#include "../../include/state/edit_state.h"
#include <QPaintEvent>
#include <QMouseEvent> 

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

    // 仅在初始化或尺寸变化时重置图像
    if (m_image.size() != size() || m_image.isNull()) {
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

    // 更新m_image以保留已提交的图形
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