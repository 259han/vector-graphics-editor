#include "ui/main_window.h"
#include "../state/draw_state.h"
#include "../state/fill_state.h"
#include "draw_area.h"
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QActionGroup>
#include <QColorDialog>
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QIcon>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QStatusBar>
#include <QDebug>
#include "../core/graphic.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_drawArea(new DrawArea(this))
    , toolsGroup(new QActionGroup(this))
    , m_currentFillColor(Qt::red) // 初始化填充颜色为红色，避免黑色不明显
{
    // 创建中心部件
    setCentralWidget(m_drawArea);
    
    // 创建动作和菜单
    createActions();
    createMenus();
    createToolBars();
    createFillSettingsDialog();
    createToolOptions();
    
    // 设置窗口标题和大小
    setWindowTitle(tr("矢量图形编辑器"));
    resize(800, 600);
}

void MainWindow::createActions() {
    // 创建选择工具
    m_selectAction = new QAction(QIcon(":/icons/select.png"), tr("选择"), this);
    m_selectAction->setCheckable(true);
    m_selectAction->setChecked(true);
    toolsGroup->addAction(m_selectAction);

    // 创建绘图工具
    m_lineAction = new QAction(QIcon(":/icons/line.png"), tr("直线"), this);
    m_lineAction->setCheckable(true);
    toolsGroup->addAction(m_lineAction);
    
    m_rectAction = new QAction(QIcon(":/icons/rectangle.png"), tr("矩形"), this);
    m_rectAction->setCheckable(true);
    toolsGroup->addAction(m_rectAction);
    
    m_ellipseAction = new QAction(QIcon(":/icons/ellipse.png"), tr("椭圆"), this);
    m_ellipseAction->setCheckable(true);
    toolsGroup->addAction(m_ellipseAction);
    
    m_bezierAction = new QAction(QIcon(":/icons/bezier.png"), tr("贝塞尔曲线"), this);
    m_bezierAction->setCheckable(true);
    toolsGroup->addAction(m_bezierAction);
    
    m_fillToolAction = new QAction(QIcon(":/icons/fill.png"), tr("填充工具"), this);
    m_fillToolAction->setCheckable(true);
    toolsGroup->addAction(m_fillToolAction);

    // 创建文件操作
    m_importImageAction = new QAction(QIcon(":/icons/import.png"), tr("导入图片"), this);
    m_saveImageAction = new QAction(QIcon(":/icons/save.png"), tr("保存图片"), this);
    m_clearAction = new QAction(QIcon(":/icons/clear.png"), tr("清空"), this);

    // 创建变换工具
    m_rotateAction = new QAction(QIcon(":/icons/rotate.png"), tr("旋转"), this);
    m_scaleAction = new QAction(QIcon(":/icons/scale.png"), tr("缩放"), this);
    m_deleteAction = new QAction(QIcon(":/icons/delete.png"), tr("删除"), this);

    // 创建图层工具
    m_bringToFrontAction = new QAction(QIcon(":/icons/front.png"), tr("置顶"), this);
    m_sendToBackAction = new QAction(QIcon(":/icons/back.png"), tr("置底"), this);
    m_bringForwardAction = new QAction(QIcon(":/icons/forward.png"), tr("上移"), this);
    m_sendBackwardAction = new QAction(QIcon(":/icons/backward.png"), tr("下移"), this);

    // 创建编辑工具
    m_copyAction = new QAction(QIcon(":/icons/copy.png"), tr("复制"), this);
    m_pasteAction = new QAction(QIcon(":/icons/paste.png"), tr("粘贴"), this);
    m_cutAction = new QAction(QIcon(":/icons/cut.png"), tr("剪切"), this);

    // 创建工具组
    toolsGroup->setExclusive(true);

    // 绘图工具信号槽
    connect(m_selectAction, &QAction::triggered, this, [this]() { onDrawActionTriggered(m_selectAction); });
    connect(m_lineAction, &QAction::triggered, this, [this]() { onDrawActionTriggered(m_lineAction); });
    connect(m_rectAction, &QAction::triggered, this, [this]() { onDrawActionTriggered(m_rectAction); });
    connect(m_ellipseAction, &QAction::triggered, this, [this]() { onDrawActionTriggered(m_ellipseAction); });
    connect(m_bezierAction, &QAction::triggered, this, [this]() { onDrawActionTriggered(m_bezierAction); });
    connect(m_fillToolAction, &QAction::triggered, this, [this]() { onDrawActionTriggered(m_fillToolAction); });
    
    // 文件操作信号槽
    connect(m_importImageAction, &QAction::triggered, m_drawArea, &DrawArea::importImage);
    connect(m_saveImageAction, &QAction::triggered, m_drawArea, &DrawArea::saveImage);
    connect(m_clearAction, &QAction::triggered, m_drawArea, &DrawArea::clearGraphics);

    // 变换工具信号槽
    connect(m_rotateAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_rotateAction); });
    connect(m_scaleAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_scaleAction); });
    connect(m_deleteAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_deleteAction); });

    // 图层工具信号槽
    connect(m_bringToFrontAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_bringToFrontAction); });
    connect(m_sendToBackAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_sendToBackAction); });
    connect(m_bringForwardAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_bringForwardAction); });
    connect(m_sendBackwardAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_sendBackwardAction); });

    // 编辑工具信号槽
    connect(m_copyAction, &QAction::triggered, this, [this]() { onEditActionTriggered(m_copyAction); });
    connect(m_pasteAction, &QAction::triggered, this, [this]() { onEditActionTriggered(m_pasteAction); });
    connect(m_cutAction, &QAction::triggered, this, [this]() { onEditActionTriggered(m_cutAction); });
}

void MainWindow::createMenus() {
    QMenu* editMenu = menuBar()->addMenu(tr("编辑"));
    editMenu->addAction(m_copyAction);
    editMenu->addAction(m_pasteAction);
    editMenu->addAction(m_cutAction);
    editMenu->addSeparator();
    editMenu->addAction(m_deleteAction);
}

void MainWindow::createToolBars() {
    // 创建绘图工具栏
    m_drawToolBar = addToolBar(tr("绘图工具"));
    m_drawToolBar->addAction(m_selectAction);
    m_drawToolBar->addSeparator();
    m_drawToolBar->addAction(m_lineAction);
    m_drawToolBar->addAction(m_rectAction);
    m_drawToolBar->addAction(m_ellipseAction);
    m_drawToolBar->addSeparator();
    m_drawToolBar->addAction(m_bezierAction);
    m_drawToolBar->addSeparator();
    m_drawToolBar->addAction(m_fillToolAction);
    m_drawToolBar->addSeparator();
    m_drawToolBar->addAction(m_importImageAction);
    m_drawToolBar->addAction(m_saveImageAction);
    m_drawToolBar->addAction(m_clearAction);

    // 创建变换工具栏
    m_transformToolBar = addToolBar(tr("变换工具"));
    m_transformToolBar->addAction(m_rotateAction);
    m_transformToolBar->addAction(m_scaleAction);
    m_transformToolBar->addAction(m_deleteAction);

    // 创建图层工具栏
    m_layerToolBar = addToolBar(tr("图层工具"));
    m_layerToolBar->addAction(m_bringToFrontAction);
    m_layerToolBar->addAction(m_sendToBackAction);
    m_layerToolBar->addAction(m_bringForwardAction);
    m_layerToolBar->addAction(m_sendBackwardAction);

    // 创建填充设置工具栏
    createFillSettingsDialog();
}

void MainWindow::createFillSettingsDialog() {
    // 创建填充设置小部件
    m_fillSettingsWidget = new QWidget(this);
    m_fillToolBar = addToolBar(tr("填充设置"));
    
    // 创建填充颜色选择按钮 - 添加到填充工具栏
    QLabel* fillColorToolbarLabel = new QLabel(tr("填充颜色:"), m_fillSettingsWidget);
    QPushButton* colorToolbarButton = new QPushButton(m_fillSettingsWidget);
    colorToolbarButton->setFixedSize(24, 24);
    colorToolbarButton->setStyleSheet(QString("background-color: %1").arg(m_currentFillColor.name()));
    connect(colorToolbarButton, &QPushButton::clicked, this, &MainWindow::onSelectFillColor);
    
    // 创建布局
    QHBoxLayout* layout = new QHBoxLayout(m_fillSettingsWidget);
    layout->addWidget(fillColorToolbarLabel);
    layout->addWidget(colorToolbarButton);
    layout->setContentsMargins(5, 0, 5, 0);
    
    // 添加到工具栏
    m_fillToolBar->addWidget(m_fillSettingsWidget);
    
    // 默认隐藏填充设置工具栏
    m_fillToolBar->setVisible(false);
}

void MainWindow::onDrawActionTriggered(QAction* action) {
    // 隐藏填充工具设置栏
    m_fillToolBar->setVisible(false);
    
    if (action == m_selectAction) {
        // 取消其他工具按钮的选中状态
        QList<QAction*> drawTools = {
            m_lineAction, m_rectAction, m_ellipseAction, m_bezierAction, m_fillToolAction
        };
        
        for (QAction* tool : drawTools) {
            tool->setChecked(false);
        }
        
        m_drawArea->setEditState();
        statusBar()->showMessage(tr("选择工具: 点击图形进行选择和编辑"));
    } else if (action == m_lineAction) {
        m_drawArea->setDrawState(Graphic::LINE);
        m_selectAction->setChecked(false);
        statusBar()->showMessage(tr("直线工具: 拖动鼠标绘制直线"));
    } else if (action == m_rectAction) {
        m_drawArea->setDrawState(Graphic::RECTANGLE);
        m_selectAction->setChecked(false);
        statusBar()->showMessage(tr("矩形工具: 拖动鼠标绘制矩形"));
    } else if (action == m_ellipseAction) {
        m_drawArea->setDrawState(Graphic::ELLIPSE);
        m_selectAction->setChecked(false);
        statusBar()->showMessage(tr("椭圆工具: 拖动鼠标绘制椭圆"));
    } else if (action == m_bezierAction) {
        m_drawArea->setDrawState(Graphic::BEZIER);
        m_selectAction->setChecked(false);
        statusBar()->showMessage(tr("贝塞尔曲线工具: 左键点击添加控制点, 右键点击完成曲线,ESC键取消"));
    } else if (action == m_fillToolAction) {
        // 填充工具的处理在 onFillToolTriggered 中进行
        onFillToolTriggered();
        return;
    }
}

void MainWindow::onTransformActionTriggered(QAction* action) {
    auto selectedItems = m_drawArea->getSelectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择图形"));
        return;
    }

    if (action == m_rotateAction) {
        m_drawArea->rotateSelectedGraphics(45.0); // 旋转45度
    } else if (action == m_scaleAction) {
        m_drawArea->scaleSelectedGraphics(1.2); // 放大1.2倍
    } else if (action == m_deleteAction) {
        m_drawArea->deleteSelectedGraphics();
    }
}

void MainWindow::onLayerActionTriggered(QAction* action) {
    auto selectedItems = m_drawArea->getSelectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择图形"));
        return;
    }

    for (auto item : selectedItems) {
        if (action == m_bringToFrontAction) {
            m_drawArea->bringToFront(item);
        } else if (action == m_sendToBackAction) {
            m_drawArea->sendToBack(item);
        } else if (action == m_bringForwardAction) {
            m_drawArea->bringForward(item);
        } else if (action == m_sendBackwardAction) {
            m_drawArea->sendBackward(item);
        }
    }
    m_drawArea->update();
}

void MainWindow::onEditActionTriggered(QAction* action) {
    auto selectedItems = m_drawArea->getSelectedItems();

    if (action == m_copyAction || action == m_cutAction) {
        if (selectedItems.isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("请先选择图形"));
            return;
        }
        
        if (action == m_copyAction) {
            m_drawArea->copySelectedItems();
        } else { // m_cutAction
            m_drawArea->cutSelectedItems();
        }
    } else if (action == m_pasteAction) {
        m_drawArea->pasteItems();
        m_drawArea->update();
    }
}

void MainWindow::onFillColorTriggered() {
    // 选择填充颜色
    QColor color = QColorDialog::getColor(m_drawArea->getFillColor(), this, tr("选择填充颜色"));
    if (color.isValid()) {
        m_drawArea->setFillColor(color);
    }
    
    // 如果是从选择颜色按钮点击的，恢复之前的工具状态
    m_fillToolAction->setChecked(false);
}

void MainWindow::onFillToolTriggered() {
    // 关闭其他工具的选中状态
    if (m_selectAction->isChecked()) m_selectAction->setChecked(false);
    if (m_lineAction->isChecked()) m_lineAction->setChecked(false);
    if (m_rectAction->isChecked()) m_rectAction->setChecked(false);
    if (m_ellipseAction->isChecked()) m_ellipseAction->setChecked(false);
    if (m_bezierAction->isChecked()) m_bezierAction->setChecked(false);
    
    // 激活填充工具
    m_fillToolAction->setChecked(true);
    
    // 设置绘图区域状态为填充
    m_drawArea->setFillState(m_currentFillColor);
    
    // 显示填充工具设置
    m_fillToolBar->setVisible(true);
    
    // 更新状态栏
    statusBar()->showMessage(tr("填充工具: 点击要填充的区域"));
    
    qDebug() << "填充工具已激活 - 颜色:" << m_currentFillColor;
}

void MainWindow::onSelectFillColor() {
    // 打开颜色选择对话框
    QColor newColor = QColorDialog::getColor(m_currentFillColor, this, tr("选择填充颜色"), 
                                           QColorDialog::ShowAlphaChannel);
    if (newColor.isValid()) {
        m_currentFillColor = newColor;
        qDebug() << "新选择的填充颜色:" << m_currentFillColor;
        
        // 更新所有显示填充颜色的按钮样式表
        QString styleSheet = QString("background-color: %1").arg(newColor.name(QColor::HexArgb));
        m_colorButton->setStyleSheet(styleSheet);
        
        // 如果fillToolBar中有颜色按钮，找到并更新它
        for (QObject* child : m_fillSettingsWidget->children()) {
            if (QPushButton* button = qobject_cast<QPushButton*>(child)) {
                if (button != m_colorButton) {
                    button->setStyleSheet(styleSheet);
                }
            }
        }
        
        // 更新绘图区域的填充颜色
        m_drawArea->setFillColor(m_currentFillColor);
        
        // 如果当前正在使用填充工具，更新状态栏
        if (m_fillToolAction->isChecked()) {
            statusBar()->showMessage(tr("填充工具: 点击要填充的区域 (颜色已更新为 %1)").arg(m_currentFillColor.name(QColor::HexArgb)));
            qDebug() << "填充颜色已更新为:" << m_currentFillColor;
        }
    }
}

void MainWindow::onGridToggled(bool enabled) {
    // Enable or disable grid in the draw area
    m_drawArea->enableGrid(enabled);
    
    // Update the status bar
    if (enabled) {
        statusBar()->showMessage(tr("网格已启用"));
    } else {
        statusBar()->showMessage(tr("网格已禁用"));
    }
    
    qDebug() << "Grid toggled:" << enabled;
}

void MainWindow::onGridSizeChanged(int size) {
    // Set the grid size in the draw area
    m_drawArea->setGridSize(size);
    
    // Update the status bar
    statusBar()->showMessage(tr("网格大小已设置为: %1").arg(size));
    
    qDebug() << "Grid size changed:" << size;
}

void MainWindow::onSnapToGridToggled(bool enabled) {
    // This would enable or disable snap-to-grid functionality
    // Since we don't have the implementation yet, just show a message
    if (enabled) {
        statusBar()->showMessage(tr("吸附到网格已启用"));
    } else {
        statusBar()->showMessage(tr("吸附到网格已禁用"));
    }
    
    qDebug() << "Snap to grid toggled:" << enabled;
}

void MainWindow::createToolOptions() {
    // 创建一个新的容器部件作为中央部件
    QWidget* container = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 首先添加DrawArea
    mainLayout->addWidget(m_drawArea);
    
    // 创建工具选项区域
    QWidget* optionsWidget = new QWidget(container);
    QHBoxLayout* optionsLayout = new QHBoxLayout(optionsWidget);
    optionsLayout->setContentsMargins(5, 5, 5, 5);
    
    // 创建填充颜色选择按钮
    QLabel* fillColorLabel = new QLabel(tr("填充颜色:"), optionsWidget);
    m_colorButton = new QPushButton(optionsWidget);
    m_colorButton->setFixedSize(24, 24);
    m_colorButton->setStyleSheet("background-color: black"); // 初始黑色
    connect(m_colorButton, &QPushButton::clicked, this, &MainWindow::onSelectFillColor);
    
    // 添加填充工具状态提示
    QLabel* fillStatusLabel = new QLabel(tr("填充工具:"), optionsWidget);
    QPushButton* fillToolButton = new QPushButton(tr("启用"), optionsWidget);
    fillToolButton->setCheckable(true);
    fillToolButton->setChecked(false);
    connect(fillToolButton, &QPushButton::clicked, this, [this, fillToolButton](bool checked) {
        if (checked) {
            // 直接调用onFillToolTriggered而不是触发action的triggered信号
            onFillToolTriggered();
            fillToolButton->setText(tr("已启用"));
        } else {
            m_selectAction->trigger(); // 切换到选择工具
            fillToolButton->setText(tr("启用"));
        }
    });
    
    // 添加工具状态连接，当填充工具状态改变时
    connect(m_fillToolAction, &QAction::toggled, fillToolButton, &QPushButton::setChecked);
    connect(m_fillToolAction, &QAction::toggled, [fillToolButton](bool checked) {
        fillToolButton->setText(checked ? tr("已启用") : tr("启用"));
    });
    
    optionsLayout->addWidget(fillColorLabel);
    optionsLayout->addWidget(m_colorButton);
    optionsLayout->addSpacing(10);
    optionsLayout->addWidget(fillStatusLabel);
    optionsLayout->addWidget(fillToolButton);
    optionsLayout->addStretch(1);
    
    // 添加工具选项区域到主布局
    mainLayout->addWidget(optionsWidget);
    
    // 设置容器为中央部件
    setCentralWidget(container);
}