#include "main_window.h"
#include "../state/draw_state.h"
#include "../state/fill_state.h"
#include "../state/edit_state.h"
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
#include "../command/command_manager.h"
#include <QApplication>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_drawArea(new DrawArea(this))
    , toolsGroup(new QActionGroup(this))
    , m_currentFillColor(Qt::green) // 初始化填充颜色为绿色
    , m_currentLineColor(Qt::blue) // 初始化线条颜色为蓝色
    , m_lineWidth(2)
    , m_updateTimer(nullptr)
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
    
    // 确保绘图区域使用了正确的初始颜色设置
    m_drawArea->setLineColor(m_currentLineColor);
    m_drawArea->setFillColor(m_currentFillColor);
    
    // 最后添加连接设置
    setupConnections();
    
    // 初始化撤销/重做按钮状态
    updateUndoRedoActions();

    // 创建更新定时器
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(100); // 100ms延迟
    connect(m_updateTimer, &QTimer::timeout, [this]() {
        CommandManager& manager = CommandManager::getInstance();
        m_undoAction->setEnabled(manager.canUndo());
        m_redoAction->setEnabled(manager.canRedo());
    });
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

    // 创建撤销/重做操作
    m_undoAction = new QAction(QIcon(":/icons/undo.png"), tr("撤销"), this);
    m_undoAction->setShortcut(QKeySequence::Undo); // Ctrl+Z
    m_undoAction->setStatusTip(tr("撤销上一步操作"));
    m_undoAction->setEnabled(false); // 初始状态下禁用
    
    m_redoAction = new QAction(QIcon(":/icons/redo.png"), tr("重做"), this);
    m_redoAction->setShortcut(QKeySequence::Redo); // Ctrl+Y
    m_redoAction->setStatusTip(tr("重做上一步撤销的操作"));
    m_redoAction->setEnabled(false); // 初始状态下禁用

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

    // 连接撤销/重做信号槽
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redo);
    
    // 连接CommandManager信号到更新UI状态的槽
    connect(&CommandManager::getInstance(), &CommandManager::commandExecuted, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::commandUndone, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::commandRedone, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::stackCleared, 
            this, &MainWindow::updateUndoRedoActions);
}

void MainWindow::createMenus() {
    QMenu* editMenu = menuBar()->addMenu(tr("编辑"));
    
    // 添加撤销/重做到编辑菜单最前面
    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    
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

    // 样式设置工具栏
    m_styleToolBar = addToolBar(tr("样式设置"));
    
    // 添加线条颜色选择按钮
    QLabel* lineColorLabel = new QLabel(tr("线条颜色:"), this);
    m_lineColorButton = new QPushButton(this);
    m_lineColorButton->setFixedSize(24, 24);
    m_lineColorButton->setStyleSheet(QString("background-color: %1").arg(m_currentLineColor.name(QColor::HexArgb)));
    connect(m_lineColorButton, &QPushButton::clicked, this, &MainWindow::onSelectLineColor);
    
    // 添加线条宽度选择
    QLabel* lineWidthLabel = new QLabel(tr("线条宽度:"), this);
    m_lineWidthSpinBox = new QSpinBox(this);
    m_lineWidthSpinBox->setRange(1, 20);
    m_lineWidthSpinBox->setValue(m_lineWidth); // 使用成员变量
    connect(m_lineWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MainWindow::onLineWidthChanged);
    
    // 添加到工具栏
    m_styleToolBar->addWidget(lineColorLabel);
    m_styleToolBar->addWidget(m_lineColorButton);
    m_styleToolBar->addSeparator();
    m_styleToolBar->addWidget(lineWidthLabel);
    m_styleToolBar->addWidget(m_lineWidthSpinBox);

    // 创建编辑工具栏
    m_editToolBar = addToolBar(tr("编辑工具"));
    m_editToolBar->addAction(m_undoAction);
    m_editToolBar->addAction(m_redoAction);
    m_editToolBar->addSeparator();
    m_editToolBar->addAction(m_copyAction);
    m_editToolBar->addAction(m_pasteAction);
    m_editToolBar->addAction(m_cutAction);
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
    // 隐藏/显示工具栏
    m_fillToolBar->setVisible(action == m_fillToolAction);
    m_styleToolBar->setVisible(action == m_selectAction);
    
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
}

void MainWindow::onSelectFillColor() {
    // 使用颜色对话框获取颜色
    QColor currentColor = m_currentFillColor;
    QColor newColor = QColorDialog::getColor(currentColor, this, tr("选择填充颜色"), 
                                       QColorDialog::ShowAlphaChannel);
    
    if (newColor.isValid() && newColor != currentColor) {
        // 保存新选择的填充颜色
        m_currentFillColor = newColor;
        
        // 更新按钮颜色
        QString styleSheet = QString("background-color: %1").arg(newColor.name(QColor::HexArgb));
        m_colorButton->setStyleSheet(styleSheet);
        
        // 检查当前是否为编辑模式，如果是则应用到选中图形
        if (m_selectAction->isChecked() && m_drawArea) {
            // 如果是编辑状态，则应用到选中的图形
            EditState* editState = dynamic_cast<EditState*>(m_drawArea->getCurrentState());
            if (editState) {
                editState->applyBrushColorChange(newColor);
                statusBar()->showMessage(tr("已更改填充颜色为 %1").arg(newColor.name(QColor::HexArgb)));
            }
        } else {
            // 更新绘图区域的填充颜色
            m_drawArea->setFillColor(m_currentFillColor);
            
            // 如果当前正在使用填充工具，更新状态栏
            if (m_fillToolAction->isChecked()) {
                statusBar()->showMessage(tr("填充工具: 点击要填充的区域 (颜色已更新为 %1)").arg(m_currentFillColor.name(QColor::HexArgb)));
            }
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
}

void MainWindow::onGridSizeChanged(int size) {
    // Set the grid size in the draw area
    m_drawArea->setGridSize(size);
    
    // Update the status bar
    statusBar()->showMessage(tr("网格大小已设置为: %1").arg(size));
}

void MainWindow::onSnapToGridToggled(bool enabled) {
    // This would enable or disable snap-to-grid functionality
    if (enabled) {
        statusBar()->showMessage(tr("吸附到网格已启用"));
    } else {
        statusBar()->showMessage(tr("吸附到网格已禁用"));
    }
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
    
    // 创建线条颜色和宽度控制
    QLabel* lineColorLabel = new QLabel(tr("线条颜色:"), optionsWidget);
    QPushButton* styleLineColorButton = new QPushButton(optionsWidget);
    styleLineColorButton->setFixedSize(24, 24);
    styleLineColorButton->setStyleSheet(QString("background-color: %1").arg(m_currentLineColor.name(QColor::HexArgb)));
    styleLineColorButton->setProperty("type", "lineColorButton");
    connect(styleLineColorButton, &QPushButton::clicked, this, &MainWindow::onSelectLineColor);
    
    QLabel* lineWidthLabel = new QLabel(tr("线条宽度:"), optionsWidget);
    QSpinBox* styleLineWidthSpinBox = new QSpinBox(optionsWidget);
    styleLineWidthSpinBox->setRange(1, 20);
    styleLineWidthSpinBox->setValue(m_lineWidth);
    styleLineWidthSpinBox->setProperty("type", "lineWidthSpinBox");
    connect(styleLineWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MainWindow::onLineWidthChanged);
    
    // 添加到布局
    optionsLayout->addWidget(lineColorLabel);
    optionsLayout->addWidget(styleLineColorButton);
    optionsLayout->addSpacing(5);
    optionsLayout->addWidget(lineWidthLabel);
    optionsLayout->addWidget(styleLineWidthSpinBox);
    optionsLayout->addSpacing(15);
    
    // 创建填充颜色选择按钮
    QLabel* fillColorLabel = new QLabel(tr("填充颜色:"), optionsWidget);
    m_colorButton = new QPushButton(optionsWidget);
    m_colorButton->setFixedSize(24, 24);
    m_colorButton->setStyleSheet(QString("background-color: %1").arg(m_currentFillColor.name(QColor::HexArgb)));
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

// 添加线条颜色选择处理函数
void MainWindow::onSelectLineColor() {
    // 使用存储的颜色，而不是尝试从按钮获取
    QColor currentColor = m_currentLineColor;
    QColor newColor = QColorDialog::getColor(currentColor, this, tr("选择线条颜色"), 
                                        QColorDialog::ShowAlphaChannel);
    
    if (newColor.isValid() && newColor != currentColor) {
        // 保存当前颜色
        m_currentLineColor = newColor;
        
        // 更新按钮颜色
        QString styleSheet = QString("background-color: %1").arg(newColor.name(QColor::HexArgb));
        m_lineColorButton->setStyleSheet(styleSheet);
        
        // 如果在工具选项区域也有相应按钮，同步更新
        for (QObject* child : findChildren<QPushButton*>()) {
            QPushButton* button = qobject_cast<QPushButton*>(child);
            if (button && button != m_lineColorButton && 
                button->property("type").toString() == "lineColorButton") {
                button->setStyleSheet(styleSheet);
            }
        }
        
        // 应用样式变更 - 使用命令模式
        if (m_selectAction->isChecked() && m_drawArea) {
            // 如果是编辑状态，则应用到选中的图形
            EditState* editState = dynamic_cast<EditState*>(m_drawArea->getCurrentState());
            if (editState) {
                editState->applyPenColorChange(newColor);
                statusBar()->showMessage(tr("已更改线条颜色为 %1").arg(newColor.name()));
            }
        } else {
            // 如果是绘制状态，则设置为绘制默认颜色
            m_drawArea->setLineColor(newColor);
            statusBar()->showMessage(tr("已设置线条颜色为 %1，将应用于新绘制的图形").arg(newColor.name()));
        }
    }
}

// 添加线条宽度变更处理函数
void MainWindow::onLineWidthChanged(int width) {
    // 保存当前宽度
    m_lineWidth = width;
    
    // 同步更新所有线宽控件
    QList<QSpinBox*> spinBoxes = findChildren<QSpinBox*>();
    for (QSpinBox* spinBox : spinBoxes) {
        if (spinBox != sender() && spinBox->property("type").toString() == "lineWidthSpinBox") {
            spinBox->blockSignals(true);
            spinBox->setValue(width);
            spinBox->blockSignals(false);
        }
    }
    
    // 应用样式变更 - 使用命令模式
    if (m_selectAction->isChecked() && m_drawArea) {
        // 如果是编辑状态，则应用到选中的图形
        EditState* editState = dynamic_cast<EditState*>(m_drawArea->getCurrentState());
        if (editState) {
            editState->applyPenWidthChange(static_cast<qreal>(width));
            statusBar()->showMessage(tr("已更改线条宽度: %1").arg(width));
        }
    } else {
        // 如果是绘制状态，则设置为绘制默认宽度
        m_drawArea->setLineWidth(width);
        statusBar()->showMessage(tr("已设置线条宽度: %1，将应用于新绘制的图形").arg(width));
    }
}

// 添加setupConnections函数
void MainWindow::setupConnections() {
    // 只保留CommandManager相关的连接
    // 撤销/重做按钮的连接已在createActions中设置
    
    // 连接CommandManager信号到更新UI状态的槽
    connect(&CommandManager::getInstance(), &CommandManager::commandExecuted, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::commandUndone, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::commandRedone, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::stackCleared, 
            this, &MainWindow::updateUndoRedoActions);
}

// 实现撤销/重做槽函数
void MainWindow::undo() {
    CommandManager::getInstance().undo();
    // updateUndoRedoActions会通过信号自动调用
    
    // 确保DrawArea更新
    if (m_drawArea) {
        m_drawArea->update();
        QApplication::processEvents();
    }
}

void MainWindow::redo() {
    CommandManager::getInstance().redo();
    // updateUndoRedoActions会通过信号自动调用
    
    // 确保DrawArea更新
    if (m_drawArea) {
        m_drawArea->update();
        QApplication::processEvents();
    }
}

void MainWindow::updateUndoRedoActions() {
    // 使用定时器延迟更新
    if (m_updateTimer) {
        m_updateTimer->start();
    }
}

void MainWindow::onFillColorTriggered()
{
    QColor color = QColorDialog::getColor(m_currentFillColor, this, tr("选择填充颜色"));
    if (color.isValid()) {
        m_currentFillColor = color;
        m_colorButton->setStyleSheet(QString("background-color: %1").arg(color.name(QColor::HexArgb)));
        m_drawArea->setFillColor(color);
    }
}