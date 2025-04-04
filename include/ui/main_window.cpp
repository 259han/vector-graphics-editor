#include "../ui/main_window.h"
#include "../state/draw_state.h"
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QActionGroup>
#include <QColorDialog>
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_drawArea = new DrawArea(this);
    setCentralWidget(m_drawArea);

    createActions();
    createToolBar();

    setWindowTitle("Vector Graphics Editor");
    resize(1024, 768);
}

void MainWindow::createActions() {
    // 线条
    m_lineAction = new QAction("Line", this);
    m_lineAction->setCheckable(true);
    connect(m_lineAction, &QAction::triggered, [this]() {
        m_drawArea->setDrawState(Graphic::LINE);
    });

    // 矩形
    m_rectangleAction = new QAction("Rectangle", this);
    m_rectangleAction->setCheckable(true);
    connect(m_rectangleAction, &QAction::triggered, [this]() {
        m_drawArea->setDrawState(Graphic::RECTANGLE);
    });

    // 圆形
    m_circleAction = new QAction("Circle", this);
    m_circleAction->setCheckable(true);
    connect(m_circleAction, &QAction::triggered, [this]() {
        m_drawArea->setDrawState(Graphic::CIRCLE);
    });

    // 三角形
    m_triangleAction = new QAction("Triangle", this);
    m_triangleAction->setCheckable(true);
    connect(m_triangleAction, &QAction::triggered, [this]() {
        m_drawArea->setDrawState(Graphic::TRIANGLE);
    });

    // 流程图节点
    m_flowchartAction = new QAction("Flowchart Node", this);
    m_flowchartAction->setCheckable(true);
    connect(m_flowchartAction, &QAction::triggered, [this]() {
        m_drawArea->setDrawState(Graphic::FLOWCHART_NODE);
    });

    // 编辑模式
    m_editAction = new QAction("Edit", this);
    m_editAction->setCheckable(true);
    connect(m_editAction, &QAction::triggered, [this]() {
        m_drawArea->setEditState();
    });

    // 椭圆
    m_ellipseAction = new QAction("Ellipse", this);
    m_ellipseAction->setCheckable(true);
    connect(m_ellipseAction, &QAction::triggered, [this]() {
        m_drawArea->setDrawState(Graphic::ELLIPSE);
    });

    // 贝塞尔曲线
    m_bezierAction = new QAction("Bezier Curve", this);
    m_bezierAction->setCheckable(true);
    connect(m_bezierAction, &QAction::triggered, [this]() {
        m_drawArea->setDrawState(Graphic::BEZIER);
    });

    m_fillColorAction = new QAction("Fill Color", this);
    m_fillColorAction->setCheckable(true);
    connect(m_fillColorAction, &QAction::triggered, [this]() {
        QColor color = QColorDialog::getColor(Qt::white, this, "Select Fill Color");
        if (color.isValid()) {
            // 取消其他工具的选中状态
            m_lineAction->setChecked(false);
            m_rectangleAction->setChecked(false);
            m_circleAction->setChecked(false);
            m_triangleAction->setChecked(false);
            m_flowchartAction->setChecked(false);
            m_editAction->setChecked(false);
            m_ellipseAction->setChecked(false);
            m_bezierAction->setChecked(false);

            // 进入填充模式
            EditorState* currentState = m_drawArea->getCurrentDrawState();
            if (auto* drawState = dynamic_cast<DrawState*>(currentState)) {
                drawState->setFillColor(color);
                drawState->setFillMode(true);
                m_fillColorAction->setChecked(true);
            }
        }
    });

    // Clear Screen
    m_clearAction = new QAction("Clear Screen", this);
    connect(m_clearAction, &QAction::triggered, [this]() {
        m_drawArea->clearGraphics();
    });

    // 导入图片
    m_importImageAction = new QAction("Import Image", this);
    connect(m_importImageAction, &QAction::triggered, [this]() {
        m_drawArea->importImage();
    });

    // 保存图片
    m_saveImageAction = new QAction("Save Image", this);
    connect(m_saveImageAction, &QAction::triggered, [this]() {
        m_drawArea->saveImage();
    });
}

void MainWindow::createToolBar() {
    m_toolBar = addToolBar("Tools");
    
    QActionGroup* drawActionGroup = new QActionGroup(this);
    drawActionGroup->addAction(m_lineAction);
    drawActionGroup->addAction(m_rectangleAction);
    drawActionGroup->addAction(m_circleAction);
    drawActionGroup->addAction(m_triangleAction);
    drawActionGroup->addAction(m_flowchartAction);
    
    // 在工具栏中添加分隔符
    m_toolBar->addSeparator(); // 添加分隔符到工具栏

    drawActionGroup->addAction(m_editAction);
    drawActionGroup->addAction(m_ellipseAction);
    drawActionGroup->addAction(m_bezierAction);
    drawActionGroup->addAction(m_fillColorAction);
    drawActionGroup->setExclusive(true);

    m_toolBar->addAction(m_lineAction);
    m_toolBar->addAction(m_rectangleAction);
    m_toolBar->addAction(m_circleAction);
    m_toolBar->addAction(m_triangleAction);
    m_toolBar->addAction(m_flowchartAction);
    
    // 添加分隔符到工具栏
    m_toolBar->addSeparator(); // 添加分隔符到工具栏

    m_toolBar->addAction(m_editAction);
    m_toolBar->addAction(m_ellipseAction);
    m_toolBar->addAction(m_bezierAction);
    m_toolBar->addAction(m_fillColorAction);
    
    // 添加分隔符到工具栏
    m_toolBar->addSeparator(); // 添加分隔符到工具栏

    m_toolBar->addAction(m_clearAction);
    m_toolBar->addAction(m_importImageAction);
    m_toolBar->addAction(m_saveImageAction);
}