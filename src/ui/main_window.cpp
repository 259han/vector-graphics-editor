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
#include <QClipboard>
#include <QElapsedTimer>
#include "../utils/performance_monitor.h"
#include <QInputDialog>
#include <QCheckBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_drawArea(new DrawArea(this))
    , toolsGroup(new QActionGroup(this))
    , m_currentFillColor(Qt::green) 
    , m_currentLineColor(Qt::darkBlue)
    , m_lineWidth(2)
    , m_updateTimer(nullptr)
    , m_isUntitled(true)
    , m_currentFilePath("")
{
    // 创建中心部件
    setCentralWidget(m_drawArea);
    
    // 创建动作和菜单
    createActions();
    createMenus();
    createToolBars();
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
    m_updateTimer->setInterval(16); //
    connect(m_updateTimer, &QTimer::timeout, [this]() {
        CommandManager& manager = CommandManager::getInstance();
        m_undoAction->setEnabled(manager.canUndo());
        m_redoAction->setEnabled(manager.canRedo());
    });
    
    // 确保性能监控选项最初是禁用的
    if (m_performanceMonitorAction) {
        m_performanceMonitorAction->setChecked(false);
    }
    // 确保性能报告按钮最初是禁用的
    if (m_showPerformanceReportAction) {
        m_showPerformanceReportAction->setEnabled(false);
    }


    // 设置性能监控
    setupPerformanceMonitoring();
}

MainWindow::~MainWindow()
{
    // 断开所有信号槽连接，防止析构时的信号触发
    this->disconnect();
    
    // 确保停止任何可能正在运行的定时器
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    
    // 确保清除选择
    if (m_drawArea) {
        // 断开绘图区域的所有连接
        m_drawArea->disconnect();
        
        // 清除选择
        m_drawArea->clearSelection();
        
        // 如果有场景，清理场景状态
        if (m_drawArea->scene()) {
            m_drawArea->scene()->clearSelection();
            m_drawArea->scene()->clearFocus();
        }
    }
    
    // 处理任何挂起的事件
    QApplication::processEvents();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // 断开关键的信号槽连接，避免窗口关闭过程中触发回调
    this->disconnect(&CommandManager::getInstance());
    
    // 禁用绘图区域的更新
    if (m_drawArea) {
        // 断开相关连接
        m_drawArea->disconnect();
        
        // 清除选择，防止析构时访问已释放的对象
        m_drawArea->clearSelection();
        
        // 清理场景状态
        if (m_drawArea->scene()) {
            m_drawArea->scene()->clearSelection();
            m_drawArea->scene()->clearFocus();
            m_drawArea->scene()->disconnect();
        }
    }
    
    // 确保处理所有挂起的事件，但避免因此创建新事件
    QApplication::processEvents();
    
    // 接受关闭事件
    event->accept();
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

    // 创建裁剪工具
    m_clipRectAction = new QAction(QIcon(":/icons/clip_rect.png"), tr("矩形裁剪"), this);
    m_clipRectAction->setCheckable(true);
    m_clipRectAction->setStatusTip(tr("使用矩形区域裁剪选中的图形"));
    toolsGroup->addAction(m_clipRectAction);
    
    m_clipFreehandAction = new QAction(QIcon(":/icons/clip_freehand.png"), tr("自由形状裁剪"), this);
    m_clipFreehandAction->setCheckable(true);
    m_clipFreehandAction->setStatusTip(tr("使用自由形状区域裁剪选中的图形"));
    toolsGroup->addAction(m_clipFreehandAction);

    // 创建文件操作
    m_importImageAction = new QAction(QIcon(":/icons/import.png"), tr("导入图片"), this);
    m_saveImageAction = new QAction(QIcon(":/icons/save.png"), tr("保存图片"), this);
    m_clearAction = new QAction(QIcon(":/icons/clear.png"), tr("清空"), this);

    // 创建变换工具
    m_rotateAction = new QAction(QIcon(":/icons/rotate.png"), tr("旋转"), this);
    m_rotateAction->setShortcut(QKeySequence("Ctrl+R"));
    m_rotateAction->setStatusTip(tr("旋转选中的图形"));
    
    m_scaleAction = new QAction(QIcon(":/icons/scale.png"), tr("缩放"), this);
    m_scaleAction->setShortcut(QKeySequence("Ctrl+T")); // 避免与保存冲突
    m_scaleAction->setStatusTip(tr("缩放选中的图形"));
    
    m_deleteAction = new QAction(QIcon(":/icons/delete.png"), tr("删除"), this);
    m_deleteAction->setShortcut(QKeySequence::Delete); // Delete键
    m_deleteAction->setStatusTip(tr("删除选中的图形"));

    // 添加翻转动作
    m_flipHorizontalAction = new QAction(QIcon(":/icons/flip_h.png"), tr("水平翻转"), this);
    m_flipHorizontalAction->setShortcut(QKeySequence("Ctrl+H"));
    m_flipHorizontalAction->setStatusTip(tr("水平翻转选中的图形"));
    
    m_flipVerticalAction = new QAction(QIcon(":/icons/flip_v.png"), tr("垂直翻转"), this);
    m_flipVerticalAction->setShortcut(QKeySequence("Ctrl+J"));
    m_flipVerticalAction->setStatusTip(tr("垂直翻转选中的图形"));

    // 创建图层工具
    m_bringToFrontAction = new QAction(QIcon(":/icons/front.png"), tr("置顶"), this);
    m_sendToBackAction = new QAction(QIcon(":/icons/back.png"), tr("置底"), this);
    m_bringForwardAction = new QAction(QIcon(":/icons/forward.png"), tr("上移"), this);
    m_sendBackwardAction = new QAction(QIcon(":/icons/backward.png"), tr("下移"), this);

    // 创建编辑工具
    m_copyAction = new QAction(QIcon(":/icons/copy.png"), tr("复制"), this);
    m_copyAction->setShortcut(QKeySequence::Copy); // Ctrl+C
    m_copyAction->setStatusTip(tr("复制选中的图形"));
    
    m_pasteAction = new QAction(QIcon(":/icons/paste.png"), tr("粘贴"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste); // Ctrl+V
    m_pasteAction->setStatusTip(tr("粘贴剪贴板中的图形"));
    
    m_cutAction = new QAction(QIcon(":/icons/cut.png"), tr("剪切"), this);
    m_cutAction->setShortcut(QKeySequence::Cut); // Ctrl+X
    m_cutAction->setStatusTip(tr("剪切选中的图形"));

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
    connect(m_fillToolAction, &QAction::triggered, this, [this]() { onFillToolTriggered(); });
    
    // 文件操作信号槽
    connect(m_importImageAction, &QAction::triggered, m_drawArea, &DrawArea::importImage);
    connect(m_saveImageAction, &QAction::triggered, m_drawArea, &DrawArea::saveImage);
    connect(m_clearAction, &QAction::triggered, m_drawArea, &DrawArea::clearGraphics);

    // 变换工具信号槽
    connect(m_rotateAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_rotateAction); });
    connect(m_scaleAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_scaleAction); });
    connect(m_deleteAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_deleteAction); });
    connect(m_flipHorizontalAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_flipHorizontalAction); });
    connect(m_flipVerticalAction, &QAction::triggered, this, [this]() { onTransformActionTriggered(m_flipVerticalAction); });

    // 图层工具信号槽
    connect(m_bringToFrontAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_bringToFrontAction); });
    connect(m_sendToBackAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_sendToBackAction); });
    connect(m_bringForwardAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_bringForwardAction); });
    connect(m_sendBackwardAction, &QAction::triggered, this, [this]() { onLayerActionTriggered(m_sendBackwardAction); });

    // 连接CommandManager信号到更新UI状态的槽
    connect(&CommandManager::getInstance(), &CommandManager::commandExecuted, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::commandUndone, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::commandRedone, 
            this, &MainWindow::updateUndoRedoActions);
    connect(&CommandManager::getInstance(), &CommandManager::stackCleared, 
            this, &MainWindow::updateUndoRedoActions);

    // 添加性能监控相关的动作
    m_performanceMonitorAction = new QAction("启用性能监控", this);
    m_performanceMonitorAction->setCheckable(true);
    m_performanceMonitorAction->setChecked(false);
    connect(m_performanceMonitorAction, &QAction::toggled, this, &MainWindow::onTogglePerformanceMonitor);
    
    // 性能报告
    m_showPerformanceReportAction = new QAction("显示性能报告", this);
    m_showPerformanceReportAction->setEnabled(false); // 初始禁用，直到启用性能监控
    connect(m_showPerformanceReportAction, &QAction::triggered, this, &MainWindow::onShowPerformanceReport);
    
    m_highQualityRenderingAction = new QAction("高质量渲染", this);
    m_highQualityRenderingAction->setCheckable(true);
    m_highQualityRenderingAction->setChecked(true);
    connect(m_highQualityRenderingAction, &QAction::toggled, this, &MainWindow::onHighQualityRendering);
    
    // 初始化网格和对齐相关动作
    m_gridAction = new QAction("显示网格", this);
    m_gridAction->setCheckable(true);
    m_gridAction->setChecked(false);
    connect(m_gridAction, &QAction::toggled, this, &MainWindow::onGridToggled);
    
    m_snapAction = new QAction("对齐到网格", this);
    m_snapAction->setCheckable(true);
    m_snapAction->setChecked(false);
    connect(m_snapAction, &QAction::toggled, this, &MainWindow::onSnapToGridToggled);
    
    // 初始化关于动作
    m_aboutAction = new QAction(tr("关于..."), this);
    connect(m_aboutAction, &QAction::triggered, this, [this]() {
    QMessageBox::about(this, tr("关于矢量图形编辑器"),
                    tr("版本 1.0\n\n"
                        "矢量图形编辑器是一个2D绘图工具，"
                        "支持多种基本图形（如直线、矩形、圆形、椭圆、Bezier曲线等）的绘制、编辑和变换。\n\n"
                        "主要功能：\n"
                        "1. 基本图形绘制：支持直线、矩形、圆形、椭圆、贝塞尔曲线等图形。\n"
                        "2. 图形编辑：提供图形的平移、旋转、缩放、镜像等操作。\n"
                        "3. 控制点操作：支持图形的控制点操作，便于精准调整。\n"
                        "4. 支持撤销重做，支持图层管理，支持图形导出为SVG格式以及自定义CVG格式。\n"
                        "5. 支持图片导入导出，图形缓存，渲染。\n\n"
                        "感谢您的使用！"));
    });

    
    m_aboutQtAction = new QAction(tr("关于Qt..."), this);
    connect(m_aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);

    // 添加导出大图像动作
    m_exportAction = new QAction(QIcon(":/icons/export.png"), tr("导出图像..."), this);
    m_exportAction->setShortcut(QKeySequence("Ctrl+E"));
    m_exportAction->setStatusTip(tr("导出图像并设置尺寸和质量"));
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::onExportImageWithOptions);

    // 添加性能优化相关动作
    m_cachingAction = new QAction(tr("启用图形缓存"), this);
    m_cachingAction->setCheckable(true);
    m_cachingAction->setChecked(false);
    connect(m_cachingAction, &QAction::toggled, this, &MainWindow::onCachingToggled);
    
    m_clippingOptimizationAction = new QAction(tr("启用视图裁剪优化"), this);
    m_clippingOptimizationAction->setCheckable(true);
    m_clippingOptimizationAction->setChecked(true);
    connect(m_clippingOptimizationAction, &QAction::toggled, this, &MainWindow::onClippingOptimizationToggled);

    // 创建文件操作动作
    m_newAction = new QAction(QIcon(":/icons/new.png"), tr("新建"), this);
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip(tr("创建新文件"));
    connect(m_newAction, &QAction::triggered, this, &MainWindow::onNewFile);
    
    m_openAction = new QAction(QIcon(":/icons/open.png"), tr("打开..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("打开已有文件"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    
    m_saveAction = new QAction(QIcon(":/icons/save.png"), tr("保存"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip(tr("保存当前文件"));
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSaveFile);
    
    m_saveAsAction = new QAction(QIcon(":/icons/saveas.png"), tr("另存为..."), this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setStatusTip(tr("以新名称保存文件"));
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::onSaveFileAs);
    
    m_exportSVGAction = new QAction(QIcon(":/icons/svg.png"), tr("导出为SVG..."), this);
    m_exportSVGAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    m_exportSVGAction->setStatusTip(tr("导出为标准SVG矢量格式"));
    connect(m_exportSVGAction, &QAction::triggered, this, &MainWindow::onExportToSVG);
    
    // 初始化最近文件动作
    for (int i = 0; i < MaxRecentFiles; ++i) {
        m_recentFileActions[i] = new QAction(this);
        m_recentFileActions[i]->setVisible(false);
        connect(m_recentFileActions[i], &QAction::triggered, this, &MainWindow::onRecentFileTriggered);
    }
    
    m_recentFileSeparator = new QAction(this);
    m_recentFileSeparator->setSeparator(true);
    m_recentFileSeparator->setVisible(false);
    
    m_clearRecentFilesAction = new QAction(tr("清除最近打开的文件"), this);
    connect(m_clearRecentFilesAction, &QAction::triggered, this, [this]() {
        m_recentFiles.clear();
        updateRecentFileActions();
    });
    m_clearRecentFilesAction->setVisible(false);
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

    // 添加视图菜单，包含性能监控选项
    QMenu* viewMenu = menuBar()->addMenu("视图");
    
    // 添加网格相关选项
    viewMenu->addAction(m_gridAction);
    viewMenu->addAction(m_snapAction);
    
    // 添加性能优化相关选项
    viewMenu->addSeparator();
    viewMenu->addAction(m_highQualityRenderingAction);
    viewMenu->addAction(m_cachingAction);
    viewMenu->addAction(m_clippingOptimizationAction);
    viewMenu->addSeparator();
    viewMenu->addAction(m_performanceMonitorAction);
    viewMenu->addAction(m_showPerformanceReportAction);

    // 创建文件菜单
    QMenu* fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(m_newAction);
    fileMenu->addAction(m_openAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addSeparator();
    
    // 添加最近文件菜单项
    for (int i = 0; i < MaxRecentFiles; ++i) {
        fileMenu->addAction(m_recentFileActions[i]);
    }
    fileMenu->addAction(m_recentFileSeparator);
    fileMenu->addAction(m_clearRecentFilesAction);
    
    fileMenu->addSeparator();
    fileMenu->addAction(m_exportAction);
    fileMenu->addAction(m_exportSVGAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_importImageAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_clearAction);

    // 创建帮助菜单
    QMenu* helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(m_aboutAction);
    helpMenu->addAction(m_aboutQtAction);
}

void MainWindow::createToolBars()
{
    // 创建主工具栏，整合绘图、编辑和文件操作
    m_drawToolBar = addToolBar(tr("主工具栏"));
    m_drawToolBar->setObjectName("mainToolBar");
    
    // 添加文件操作按钮
    m_drawToolBar->addAction(m_importImageAction);
    m_drawToolBar->addAction(m_saveImageAction);
    m_drawToolBar->addAction(m_clearAction);
    m_drawToolBar->addSeparator();

    // 添加绘图工具
    m_drawToolBar->addAction(m_selectAction);
    m_drawToolBar->addAction(m_lineAction);
    m_drawToolBar->addAction(m_rectAction);
    m_drawToolBar->addAction(m_ellipseAction);
    m_drawToolBar->addAction(m_bezierAction);
    m_drawToolBar->addAction(m_fillToolAction);
    m_drawToolBar->addSeparator();

    // 添加裁剪工具
    m_drawToolBar->addSeparator();
    m_drawToolBar->addAction(m_clipRectAction);
    m_drawToolBar->addAction(m_clipFreehandAction);
    
    // 创建变换工具栏
    m_transformToolBar = addToolBar(tr("变换工具栏"));
    m_transformToolBar->setObjectName("transformToolBar");
    
    // 添加变换工具
    m_transformToolBar->addAction(m_rotateAction);
    m_transformToolBar->addAction(m_scaleAction);
    m_transformToolBar->addAction(m_flipHorizontalAction);
    m_transformToolBar->addAction(m_flipVerticalAction);
    m_transformToolBar->addAction(m_deleteAction);
    
    // 创建图层工具栏
    m_layerToolBar = addToolBar(tr("图层工具栏"));
    m_layerToolBar->setObjectName("layerToolBar");
    
    // 添加图层工具
    m_layerToolBar->addAction(m_bringToFrontAction);
    m_layerToolBar->addAction(m_sendToBackAction);
    m_layerToolBar->addAction(m_bringForwardAction);
    m_layerToolBar->addAction(m_sendBackwardAction);
    
    // 创建填充工具栏
    m_fillToolBar = addToolBar(tr("填充工具栏"));
    m_fillToolBar->setObjectName("fillToolBar");
    
    // 添加填充工具
    QLabel* fillColorLabel = new QLabel(tr("填充:"), this);
    m_colorButton = new QPushButton(this);
    m_colorButton->setFixedSize(24, 24);
    m_colorButton->setStyleSheet(QString("background-color: %1").arg(m_currentFillColor.name(QColor::HexArgb)));
    connect(m_colorButton, &QPushButton::clicked, this, &MainWindow::onSelectFillColor);
    
    m_fillToolBar->addWidget(fillColorLabel);
    m_fillToolBar->addWidget(m_colorButton);
    
    // 创建样式工具栏
    m_styleToolBar = addToolBar(tr("样式工具栏"));
    m_styleToolBar->setObjectName("styleToolBar");
    
    // 添加线条颜色选择按钮
    QLabel* lineColorLabel = new QLabel(tr("线条:"), this);
    m_lineColorButton = new QPushButton(this);
    m_lineColorButton->setFixedSize(24, 24);
    m_lineColorButton->setStyleSheet(QString("background-color: %1").arg(m_currentLineColor.name(QColor::HexArgb)));
    connect(m_lineColorButton, &QPushButton::clicked, this, &MainWindow::onSelectLineColor);
    
    // 添加线条宽度选择
    m_lineWidthSpinBox = new QSpinBox(this);
    m_lineWidthSpinBox->setRange(1, 20);
    m_lineWidthSpinBox->setValue(m_lineWidth);
    m_lineWidthSpinBox->setMaximumWidth(50);
    connect(m_lineWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &MainWindow::onLineWidthChanged);
    
    // 添加到工具栏
    m_styleToolBar->addWidget(lineColorLabel);
    m_styleToolBar->addWidget(m_lineColorButton);
    m_styleToolBar->addWidget(m_lineWidthSpinBox);
    
    // 创建编辑工具栏
    m_editToolBar = addToolBar(tr("编辑工具栏"));
    m_editToolBar->setObjectName("editToolBar");
    
    // 添加编辑工具
    m_editToolBar->addAction(m_undoAction);
    m_editToolBar->addAction(m_redoAction);
    m_editToolBar->addSeparator();
    m_editToolBar->addAction(m_copyAction);
    m_editToolBar->addAction(m_pasteAction);
    m_editToolBar->addAction(m_cutAction);
    
    // 创建流程图工具栏
    createFlowchartToolbar();
}

void MainWindow::createToolOptions() {
    // 创建一个新的容器部件作为中央部件
    QWidget* container = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 首先添加DrawArea
    mainLayout->addWidget(m_drawArea);
    
    // 创建状态栏上方的紧凑工具选项区域
    QWidget* optionsWidget = new QWidget(container);
    optionsWidget->setMaximumHeight(40); // 限制最大高度，保持紧凑
    QHBoxLayout* optionsLayout = new QHBoxLayout(optionsWidget);
    optionsLayout->setContentsMargins(5, 2, 5, 2);
    
    // 1. 添加视图缩放控制
    QLabel* zoomLabel = new QLabel(tr("缩放:"), optionsWidget);
    zoomLabel->setToolTip(tr("调整视图缩放比例"));
    
    QSlider* zoomSlider = new QSlider(Qt::Horizontal, optionsWidget);
    zoomSlider->setRange(50, 200);
    zoomSlider->setValue(100);
    zoomSlider->setMaximumWidth(100);
    zoomSlider->setToolTip(tr("拖动调整缩放比例"));
    connect(zoomSlider, &QSlider::valueChanged, this, [this](int value){
        if (m_drawArea) {
            // 将滑块值转换为缩放因子
            qreal factor = value / 100.0;
            // 设置视图变换
            m_drawArea->resetTransform();
            m_drawArea->scale(factor, factor);
            
            // 更新状态栏显示
            statusBar()->showMessage(tr("缩放比例: %1%").arg(value), 2000);
        }
    });
    
    // 2. 添加网格控制
    QCheckBox* gridCheckBox = new QCheckBox(tr("网格"), optionsWidget);
    gridCheckBox->setChecked(false);
    gridCheckBox->setToolTip(tr("显示/隐藏网格"));
    connect(gridCheckBox, &QCheckBox::toggled, this, &MainWindow::onGridToggled);
    
    // 3. 添加高质量渲染切换
    QCheckBox* qualityCheckBox = new QCheckBox(tr("高质量"), optionsWidget);
    qualityCheckBox->setChecked(true);
    qualityCheckBox->setToolTip(tr("切换高质量渲染"));
    connect(qualityCheckBox, &QCheckBox::toggled, m_highQualityRenderingAction, &QAction::setChecked);
    
    // 4. 添加性能优化切换
    QCheckBox* cachingCheckBox = new QCheckBox(tr("缓存"), optionsWidget);
    cachingCheckBox->setChecked(false);
    cachingCheckBox->setToolTip(tr("启用图形缓存以提高性能"));
    connect(cachingCheckBox, &QCheckBox::toggled, m_cachingAction, &QAction::setChecked);
    
    // 将控件添加到布局
    optionsLayout->addWidget(zoomLabel);
    optionsLayout->addWidget(zoomSlider);
    optionsLayout->addSpacing(15);
    optionsLayout->addWidget(gridCheckBox);
    optionsLayout->addSpacing(15);
    optionsLayout->addWidget(qualityCheckBox);
    optionsLayout->addWidget(cachingCheckBox);
    optionsLayout->addStretch(1);
    
    // 添加状态指示器
    QLabel* statusLabel = new QLabel(tr("状态:"), optionsWidget);
    QLabel* currentToolLabel = new QLabel(tr("选择工具"), optionsWidget);
    currentToolLabel->setStyleSheet("font-weight: bold;");
    
    // 更新当前工具指示器
    auto updateToolIndicator = [currentToolLabel](QAction* action, const QString& name) {
        if (action && action->isChecked()) {
            currentToolLabel->setText(name);
        }
    };
    
    connect(m_selectAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) updateToolIndicator(m_selectAction, tr("选择工具"));
    });
    connect(m_lineAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) updateToolIndicator(m_lineAction, tr("直线工具"));
    });
    connect(m_rectAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) updateToolIndicator(m_rectAction, tr("矩形工具"));
    });
    connect(m_ellipseAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) updateToolIndicator(m_ellipseAction, tr("椭圆工具"));
    });
    connect(m_bezierAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) updateToolIndicator(m_bezierAction, tr("贝塞尔工具"));
    });
    connect(m_fillToolAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) updateToolIndicator(m_fillToolAction, tr("填充工具"));
    });
    
    optionsLayout->addWidget(statusLabel);
    optionsLayout->addWidget(currentToolLabel);
    
    // 添加工具选项区域到主布局
    mainLayout->addWidget(optionsWidget);
    
    // 设置容器为中央部件
    setCentralWidget(container);
    
    // 连接高质量渲染动作和复选框的同步
    connect(m_highQualityRenderingAction, &QAction::toggled, qualityCheckBox, &QCheckBox::setChecked);
    connect(m_cachingAction, &QAction::toggled, cachingCheckBox, &QCheckBox::setChecked);
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
    // 绘图区域的连接
    connect(m_drawArea, &DrawArea::selectionChanged, this, &MainWindow::updateActionStates);
    
    // 连接DrawArea的状态消息信号到状态栏
    connect(m_drawArea, &DrawArea::statusMessageChanged, this, 
            [this](const QString& message, int timeout) {
                statusBar()->showMessage(message, timeout);
            });
    
    // 撤销/重做按钮连接到CommandManager
    CommandManager& cmdManager = CommandManager::getInstance();
    connect(&cmdManager, &CommandManager::commandExecuted, this, &MainWindow::updateUndoRedoActions);
    connect(&cmdManager, &CommandManager::commandUndone, this, &MainWindow::updateUndoRedoActions);
    connect(&cmdManager, &CommandManager::commandRedone, this, &MainWindow::updateUndoRedoActions);
    connect(&cmdManager, &CommandManager::stackCleared, this, &MainWindow::updateUndoRedoActions);
    
    // 撤销/重做动作连接
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redo);
    
    // 剪贴板操作连接
    connect(m_copyAction, &QAction::triggered, this, [this]() { onEditActionTriggered(m_copyAction); });
    connect(m_pasteAction, &QAction::triggered, this, [this]() { onEditActionTriggered(m_pasteAction); });
    connect(m_cutAction, &QAction::triggered, this, [this]() { onEditActionTriggered(m_cutAction); });
    
    // 监听系统剪贴板变化
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateClipboardActions);
    
    // 裁剪工具连接
    connect(m_clipRectAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) onClipToolTriggered(false); // 矩形裁剪
    });
    
    connect(m_clipFreehandAction, &QAction::toggled, this, [=](bool checked) {
        if (checked) onClipToolTriggered(true); // 自由形状裁剪
    });
    
    // 初始化动作状态
    updateActionStates();
    
    // 流程图工具连接
    connect(m_flowchartProcessAction, &QAction::triggered, this, [this]() {
        m_drawArea->setDrawState(GraphicItem::FLOWCHART_PROCESS);
        statusBar()->showMessage("已选择流程图处理框工具", 3000);
    });
    
    connect(m_flowchartDecisionAction, &QAction::triggered, this, [this]() {
        m_drawArea->setDrawState(GraphicItem::FLOWCHART_DECISION);
        statusBar()->showMessage("已选择流程图判断框工具", 3000);
    });
    
    connect(m_flowchartStartEndAction, &QAction::triggered, this, [this]() {
        m_drawArea->setDrawState(GraphicItem::FLOWCHART_START_END);
        statusBar()->showMessage("已选择流程图开始/结束框工具", 3000);
    });
    
    connect(m_flowchartIOAction, &QAction::triggered, this, [this]() {
        m_drawArea->setDrawState(GraphicItem::FLOWCHART_IO);
        statusBar()->showMessage("已选择流程图输入/输出框工具", 3000);
    });
    
    connect(m_flowchartConnectorAction, &QAction::triggered, this, [this]() {
        m_drawArea->setDrawState(GraphicItem::FLOWCHART_CONNECTOR);
        statusBar()->showMessage("已选择流程图连接器工具", 3000);
    });
    
    connect(m_autoConnectAction, &QAction::triggered, this, [this]() {
        m_drawArea->setAutoConnectState();
        statusBar()->showMessage("已进入自动连接模式 - 点击流程图元素的连接点来创建连接", 3000);
    });
    
    // 连接器样式选择连接
    connect(m_connectorTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this](int index) {
        // 在绘图区域保存当前选择的连接器类型
        m_drawArea->setConnectorType(static_cast<FlowchartConnectorItem::ConnectorType>(index));
    });
    
    connect(m_arrowTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, [this](int index) {
        // 在绘图区域保存当前选择的箭头类型
        m_drawArea->setArrowType(static_cast<FlowchartConnectorItem::ArrowType>(index));
    });
}

// Add this method before updateUndoRedoActions
void MainWindow::updateActionStates() {
    // 获取选中项数量
    int selectedItemCount = m_drawArea->getSelectedItems().size();
    bool hasSelection = selectedItemCount > 0;
    
    // 更新依赖选择的动作状态
    m_copyAction->setEnabled(hasSelection);
    m_cutAction->setEnabled(hasSelection);
    
    // 更新其他动作
    m_deleteAction->setEnabled(hasSelection);
    m_rotateAction->setEnabled(hasSelection);
    m_scaleAction->setEnabled(hasSelection);
    m_flipHorizontalAction->setEnabled(hasSelection);
    m_flipVerticalAction->setEnabled(hasSelection);
    
    // 图层操作
    m_bringToFrontAction->setEnabled(hasSelection);
    m_sendToBackAction->setEnabled(hasSelection);
    m_bringForwardAction->setEnabled(hasSelection);
    m_sendBackwardAction->setEnabled(hasSelection);
    
    // 更新剪贴板状态
    updateClipboardActions();
}

// Add this method after updateActionStates
void MainWindow::updateClipboardActions() {
    // 检查是否可以从剪贴板粘贴
    bool canPaste = m_drawArea->canPasteFromClipboard();
    m_pasteAction->setEnabled(canPaste);
    
    // 状态栏提示
    if (canPaste) {
        statusBar()->showMessage(tr("剪贴板中有可粘贴的图形"), 3000);
    }
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

//图层控制
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

void MainWindow::onTransformActionTriggered(QAction* action) {
    auto selectedItems = m_drawArea->getSelectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择图形"));
        return;
    }

    if (action == m_rotateAction) {
        // 创建一个菜单提供多种旋转选项
        QMenu rotateMenu;
        QAction* rotate45Action = rotateMenu.addAction(tr("旋转45度"));
        QAction* rotate90Action = rotateMenu.addAction(tr("旋转90度"));
        QAction* rotate180Action = rotateMenu.addAction(tr("旋转180度"));
        rotateMenu.addSeparator();
        QAction* customRotateAction = rotateMenu.addAction(tr("自定义角度..."));
        
        QAction* selectedAction = rotateMenu.exec(QCursor::pos());
        
        if (selectedAction == rotate45Action) {
            m_drawArea->rotateSelectedGraphics(45.0);
            statusBar()->showMessage(tr("已旋转选中图形 45 度"), 3000);
        } else if (selectedAction == rotate90Action) {
            m_drawArea->rotateSelectedGraphics(90.0);
            statusBar()->showMessage(tr("已旋转选中图形 90 度"), 3000);
        } else if (selectedAction == rotate180Action) {
            m_drawArea->rotateSelectedGraphics(180.0);
            statusBar()->showMessage(tr("已旋转选中图形 180 度"), 3000);
        } else if (selectedAction == customRotateAction) {
            // 使用输入对话框获取旋转角度
            bool ok;
            double angle = QInputDialog::getDouble(this, tr("旋转"),
                                              tr("请输入旋转角度（顺时针为正，逆时针为负）:"),
                                              45.0, -360.0, 360.0, 1, &ok);
            if (ok && angle != 0.0) {
                m_drawArea->rotateSelectedGraphics(angle);
                statusBar()->showMessage(tr("已旋转选中图形 %.1f 度").arg(angle), 3000);
            }
        }
    } else if (action == m_scaleAction) {
        // 创建一个菜单提供多种缩放选项
        QMenu scaleMenu;
        QAction* enlargeAction = scaleMenu.addAction(tr("放大 (×1.2)"));
        QAction* shrinkAction = scaleMenu.addAction(tr("缩小 (×0.8)"));
        QAction* doubleAction = scaleMenu.addAction(tr("放大一倍 (×2)"));
        QAction* halfAction = scaleMenu.addAction(tr("缩小一半 (×0.5)"));
        scaleMenu.addSeparator();
        QAction* customScaleAction = scaleMenu.addAction(tr("自定义比例..."));
        
        QAction* selectedAction = scaleMenu.exec(QCursor::pos());
        
        if (selectedAction == enlargeAction) {
            m_drawArea->scaleSelectedGraphics(1.2);
            statusBar()->showMessage(tr("已放大选中图形 1.2 倍"), 3000);
        } else if (selectedAction == shrinkAction) {
            m_drawArea->scaleSelectedGraphics(0.8);
            statusBar()->showMessage(tr("已缩小选中图形 0.8 倍"), 3000);
        } else if (selectedAction == doubleAction) {
            m_drawArea->scaleSelectedGraphics(2.0);
            statusBar()->showMessage(tr("已放大选中图形 2 倍"), 3000);
        } else if (selectedAction == halfAction) {
            m_drawArea->scaleSelectedGraphics(0.5);
            statusBar()->showMessage(tr("已缩小选中图形 0.5 倍"), 3000);
        } else if (selectedAction == customScaleAction) {
            // 使用输入对话框获取缩放比例
            bool ok;
            double factor = QInputDialog::getDouble(this, tr("缩放"),
                                              tr("请输入缩放比例（大于1放大，小于1缩小）:"),
                                              1.2, 0.1, 10.0, 2, &ok);
            if (ok && factor != 1.0) {
                m_drawArea->scaleSelectedGraphics(factor);
                statusBar()->showMessage(tr("已缩放选中图形 %.2f 倍").arg(factor), 3000);
            }
        }
    } else if (action == m_deleteAction) {
        m_drawArea->deleteSelectedGraphics();
        statusBar()->showMessage(tr("已删除选中图形"), 3000);
    } else if (action == m_flipHorizontalAction) {
        m_drawArea->flipSelectedGraphics(true); // 水平翻转
        statusBar()->showMessage(tr("已水平翻转选中图形"), 3000);
    } else if (action == m_flipVerticalAction) {
        m_drawArea->flipSelectedGraphics(false); // 垂直翻转
        statusBar()->showMessage(tr("已垂直翻转选中图形"), 3000);
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

// 性能监控相关槽函数实现
void MainWindow::onTogglePerformanceMonitor(bool checked)
{
    // 更新性能报告按钮状态
    m_showPerformanceReportAction->setEnabled(checked);
    
    // 立即给用户反馈
    statusBar()->showMessage(checked ? "正在启用性能监控..." : "正在禁用性能监控...");
    
    try {
        // 调用异步的 enablePerformanceMonitor 方法，传递 checked 状态
        m_drawArea->enablePerformanceMonitor(checked);
        
        // 更新状态栏消息，成功时显示 3 秒
        statusBar()->showMessage(checked ? "性能监控已启用" : "性能监控已禁用", 3000);
    }
    catch (const std::exception& e) {
        // 出现异常时恢复 UI 状态
        m_performanceMonitorAction->setChecked(!checked); // 恢复到之前的状态
        m_showPerformanceReportAction->setEnabled(!checked);
        
        // 根据 checked 值动态生成错误消息
        QString errorMsg = QString("%1性能监控时发生错误: %2")
                           .arg(checked ? "启用" : "禁用")
                           .arg(e.what());
        statusBar()->showMessage(errorMsg, 5000);
        Logger::error(errorMsg);
        
        QMessageBox::critical(this, "错误", errorMsg);
    }
    catch (...) {
        // 处理未知异常
        m_performanceMonitorAction->setChecked(!checked);
        m_showPerformanceReportAction->setEnabled(!checked);
        
        QString errorMsg = QString("%1性能监控时发生未知错误")
                           .arg(checked ? "启用" : "禁用");
        statusBar()->showMessage(errorMsg, 5000);
        Logger::error(errorMsg);
        
        QMessageBox::critical(this, "错误", errorMsg);
    }
}

void MainWindow::onShowPerformanceReport()
{
    QString report = m_drawArea->getPerformanceReport();
    
    // 创建自定义对话框
    QDialog reportDialog(this);
    reportDialog.setWindowTitle("性能报告");
    reportDialog.setMinimumSize(800, 600);
    
    // 创建文本编辑器和布局
    QVBoxLayout* layout = new QVBoxLayout(&reportDialog);
    QTextEdit* textEdit = new QTextEdit(&reportDialog);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Consolas, Courier New, Monospace", 10));
    textEdit->setText(report);
    
    // 添加保存按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* saveButton = new QPushButton("保存报告", &reportDialog);
    QPushButton* copyButton = new QPushButton("复制到剪贴板", &reportDialog);
    QPushButton* closeButton = new QPushButton("关闭", &reportDialog);
    
    connect(saveButton, &QPushButton::clicked, [&]() {
        QString filePath = QFileDialog::getSaveFileName(
            this, "保存性能报告", 
            QDir::homePath() + "/performance_report_" + 
            QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".txt",
            "文本文件 (*.txt);;所有文件 (*)");
            
        if (!filePath.isEmpty()) {
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << report;
                file.close();
                QMessageBox::information(this, "保存成功", "性能报告已保存到：" + filePath);
            } else {
                QMessageBox::critical(this, "保存失败", "无法写入文件：" + filePath);
            }
        }
    });
    
    connect(copyButton, &QPushButton::clicked, [&]() {
        QApplication::clipboard()->setText(report);
        QMessageBox::information(this, "复制成功", "性能报告已复制到剪贴板");
    });
    
    connect(closeButton, &QPushButton::clicked, &reportDialog, &QDialog::accept);
    
    btnLayout->addWidget(saveButton);
    btnLayout->addWidget(copyButton);
    btnLayout->addStretch();
    btnLayout->addWidget(closeButton);
    
    layout->addWidget(textEdit);
    layout->addLayout(btnLayout);
    
    // 显示对话框
    reportDialog.exec();
}

void MainWindow::onHighQualityRendering(bool checked)
{
    // 使用作用域计时器测量整个函数执行时间
    PERF_SCOPE(LogicTime);
    
    // 设置高质量渲染
    m_drawArea->setHighQualityRendering(checked);
    
    // 记录自定义事件
    PERF_EVENT("QualityChanged", checked ? 1 : 0);
}

// 实现createPerformanceMenu方法，由于已在createMenus中直接实现，此处留空
void MainWindow::createPerformanceMenu()
{
    // 性能菜单已在createMenus()方法中创建
    // 此处仅为了解决链接错误而保留空实现
}

// 实现在MainWindow中使用轻量级性能监控的示例函数
void MainWindow::setupPerformanceMonitoring()
{
    // 注册自定义指标回调 - 监控应用内存使用
    PerformanceMonitor::instance().registerMetricCallback("MainWindowMetrics", [](QMap<QString, QVariant>& metrics) {
        // 可以在这里添加应用特定的指标收集
        // 例如：当前打开的文件数、图形对象数等
        QApplication* app = qApp;
        if (app) {
            // 获取活跃的窗口数
            metrics["ActiveWindows"] = app->topLevelWindows().size();
            
            // 获取窗口小部件数量
            metrics["WidgetCount"] = app->allWidgets().size();
        }
    });
    
    // 连接数据更新信号，当性能数据更新时可以做其他操作
    connect(&PerformanceMonitor::instance(), &PerformanceMonitor::dataUpdated, 
            this, [this]() {
                // 仅在性能监控启用时执行
                if (!PerformanceMonitor::instance().isEnabled()) {
                    return;
                }
                
                try {
                    // 性能指标合格性检查
                    int fps = PerformanceMonitor::instance().getFPS();
                    double drawTime = PerformanceMonitor::instance().getAverageTime(PerformanceMonitor::DrawTime);
                    
                    // 帧率过低警告
                    if (fps < 30) {
                        statusBar()->showMessage("警告: 帧率较低 (" + QString::number(fps) + " FPS)", 2000);
                    }
                    
                    
                    // 绘制时间过长警告
                    if (drawTime > 16.0) {  // 16ms对应60fps
                        statusBar()->showMessage("警告: 绘制时间较长 (" + QString::number(drawTime, 'f', 1) + "ms)", 2000);
                    }
                } catch (const std::exception& e) {
                    // 捕获异常，防止崩溃
                    qDebug() << "性能数据处理异常:" << e.what();
                }
            });
}

// 添加性能优化相关槽函数实现
void MainWindow::onCachingToggled(bool checked)
{
    if (m_drawArea) {
        m_drawArea->enableGraphicsCaching(checked);
        
        // 更新状态栏信息
        QString message = checked ? "已启用图形缓存，性能将提升" : "已禁用图形缓存";
        statusBar()->showMessage(message, 3000);
    }
}

void MainWindow::onClippingOptimizationToggled(bool checked)
{
    if (m_drawArea) {
        m_drawArea->enableClippingOptimization(checked);
        
        // 更新状态栏信息
        QString message = checked ? "已启用视图裁剪优化，仅渲染可见图形" : "已禁用视图裁剪优化";
        statusBar()->showMessage(message, 3000);
    }
}

void MainWindow::onExportImageWithOptions()
{
    if (m_drawArea) {
        m_drawArea->saveImageWithOptions();
    }
}

void MainWindow::onDrawActionTriggered(QAction* action) {
    // 根据不同的工具设置不同的绘图状态
    if (action == m_selectAction) {
        m_drawArea->setEditState();
        statusBar()->showMessage(tr("选择工具: 点击选择图形，拖动移动图形"));
    } else if (action == m_lineAction) {
        m_drawArea->setDrawState(Graphic::LINE);
        m_drawArea->setLineColor(m_currentLineColor);
        m_drawArea->setLineWidth(m_lineWidth);
        m_drawArea->setFillColor(m_currentFillColor);
        statusBar()->showMessage(tr("直线工具: 点击并拖动绘制直线"));
    } else if (action == m_rectAction) {
        m_drawArea->setDrawState(Graphic::RECTANGLE);
        m_drawArea->setLineColor(m_currentLineColor);
        m_drawArea->setLineWidth(m_lineWidth);
        m_drawArea->setFillColor(m_currentFillColor);
        statusBar()->showMessage(tr("矩形工具: 点击并拖动绘制矩形"));
    } else if (action == m_ellipseAction) {
        m_drawArea->setDrawState(Graphic::ELLIPSE);
        m_drawArea->setLineColor(m_currentLineColor);
        m_drawArea->setLineWidth(m_lineWidth);
        m_drawArea->setFillColor(m_currentFillColor);
        statusBar()->showMessage(tr("椭圆工具: 点击并拖动绘制椭圆"));
    } else if (action == m_bezierAction) {
        m_drawArea->setDrawState(Graphic::BEZIER);
        m_drawArea->setLineColor(m_currentLineColor);
        m_drawArea->setLineWidth(m_lineWidth);
        m_drawArea->setFillColor(m_currentFillColor);
        statusBar()->showMessage(tr("贝塞尔曲线工具: 点击创建控制点，双击结束"));
    }
}

// 新建文件
void MainWindow::onNewFile() {
    // 询问是否保存当前文件
    if (!m_isUntitled || m_drawArea->scene()->items().count() > 0) {
        QMessageBox::StandardButton ret = QMessageBox::warning(
            this, tr("新建文件"),
            tr("当前文件尚未保存，是否保存更改？"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );
        
        if (ret == QMessageBox::Save) {
            onSaveFile();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }
    
    // 创建新文件
    m_drawArea->clearGraphics();
    m_currentFilePath.clear();
    m_isUntitled = true;
    setWindowTitle(tr("无标题 - 矢量图形编辑器"));
    statusBar()->showMessage(tr("已创建新文件"), 2000);
}

// 打开文件
void MainWindow::onOpenFile() {
    // 询问是否保存当前文件
    if (!m_isUntitled || m_drawArea->scene()->items().count() > 0) {
        QMessageBox::StandardButton ret = QMessageBox::warning(
            this, tr("打开文件"),
            tr("当前文件尚未保存，是否保存更改？"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );
        
        if (ret == QMessageBox::Save) {
            onSaveFile();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }
    
    // 打开文件
    m_drawArea->openWithFormatDialog();
    // 此处假设打开成功，如果需要可以添加成功标志返回值
    m_isUntitled = false;
    m_currentFilePath = ""; // TODO: 获取实际文件路径
    
    updateRecentFileActions();
}

// 保存文件
void MainWindow::onSaveFile() {
    if (m_isUntitled) {
        // 如果是无标题文件，转到另存为
        onSaveFileAs();
    } else {
        // 检查文件类型
        QFileInfo info(m_currentFilePath);
        QString suffix = info.suffix().toLower();
        
        bool success = false;
        if (suffix == "cvg") {
            success = m_drawArea->saveToCustomFormat(m_currentFilePath);
        } else {
            // 其他格式用另存为对话框
            onSaveFileAs();
            return;
        }
        
        if (success) {
            statusBar()->showMessage(tr("文件已保存"), 2000);
            
            // 添加到最近文件列表
            addToRecentFiles(m_currentFilePath);
        }
    }
}

// 另存为
void MainWindow::onSaveFileAs() {
    // 使用DrawArea中的格式选择对话框
    m_drawArea->saveAsWithFormatDialog();
    // TODO: 获取实际保存的文件路径
    
    m_isUntitled = false;
    updateWindowTitle();
}

// 导出SVG
void MainWindow::onExportToSVG() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("导出为SVG"),
        QDir::homePath(), tr("SVG文件 (*.svg)"));
    
    if (fileName.isEmpty()) {
        return;
    }
    
    if (!fileName.endsWith(".svg", Qt::CaseInsensitive)) {
        fileName += ".svg";
    }
    
    // 创建尺寸设置对话框
    QDialog sizeDialog(this);
    sizeDialog.setWindowTitle(tr("设置SVG尺寸"));
    
    QVBoxLayout* layout = new QVBoxLayout(&sizeDialog);
    
    // 场景尺寸
    QRectF sceneRect = m_drawArea->scene()->sceneRect();
    int defaultWidth = sceneRect.width();
    int defaultHeight = sceneRect.height();
    
    // 宽度设置
    QHBoxLayout* widthLayout = new QHBoxLayout();
    QLabel* widthLabel = new QLabel(tr("宽度:"));
    QSpinBox* widthSpin = new QSpinBox();
    widthSpin->setRange(1, 10000);
    widthSpin->setValue(defaultWidth);
    widthSpin->setSuffix(tr(" px"));
    widthLayout->addWidget(widthLabel);
    widthLayout->addWidget(widthSpin);
    
    // 高度设置
    QHBoxLayout* heightLayout = new QHBoxLayout();
    QLabel* heightLabel = new QLabel(tr("高度:"));
    QSpinBox* heightSpin = new QSpinBox();
    heightSpin->setRange(1, 10000);
    heightSpin->setValue(defaultHeight);
    heightSpin->setSuffix(tr(" px"));
    heightLayout->addWidget(heightLabel);
    heightLayout->addWidget(heightSpin);
    
    // 保持比例
    QCheckBox* keepAspectRatio = new QCheckBox(tr("保持宽高比"));
    keepAspectRatio->setChecked(true);
    
    // 连接保持比例信号
    double aspectRatio = static_cast<double>(defaultWidth) / defaultHeight;
    connect(widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            [&](int value) {
                if (keepAspectRatio->isChecked()) {
                    heightSpin->blockSignals(true);
                    heightSpin->setValue(qRound(value / aspectRatio));
                    heightSpin->blockSignals(false);
                }
            });
    
    connect(heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), 
            [&](int value) {
                if (keepAspectRatio->isChecked()) {
                    widthSpin->blockSignals(true);
                    widthSpin->setValue(qRound(value * aspectRatio));
                    widthSpin->blockSignals(false);
                }
            });
    
    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &sizeDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &sizeDialog, &QDialog::reject);
    
    // 添加到布局
    layout->addLayout(widthLayout);
    layout->addLayout(heightLayout);
    layout->addWidget(keepAspectRatio);
    layout->addWidget(buttonBox);
    
    // 执行对话框
    if (sizeDialog.exec() != QDialog::Accepted) {
        return;
    }
    
    // 获取设置的尺寸并导出
    QSize size(widthSpin->value(), heightSpin->value());
    if (m_drawArea->exportToSVG(fileName, size)) {
        statusBar()->showMessage(tr("SVG已导出: %1").arg(fileName), 3000);
        
        // 可选：打开导出的文件
        QMessageBox::StandardButton ret = QMessageBox::question(
            this, tr("导出成功"),
            tr("SVG文件已成功导出。是否打开该文件？"),
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (ret == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
        }
    }
}

// 处理最近文件菜单项点击
void MainWindow::onRecentFileTriggered() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString filePath = action->data().toString();
        if (QFile::exists(filePath)) {
            // 询问是否保存当前文件
            if (!m_isUntitled || m_drawArea->scene()->items().count() > 0) {
                QMessageBox::StandardButton ret = QMessageBox::warning(
                    this, tr("打开文件"),
                    tr("当前文件尚未保存，是否保存更改？"),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
                );
                
                if (ret == QMessageBox::Save) {
                    onSaveFile();
                } else if (ret == QMessageBox::Cancel) {
                    return;
                }
            }
            
            // 根据文件类型打开
            QFileInfo info(filePath);
            QString suffix = info.suffix().toLower();
            
            if (suffix == "cvg") {
                m_drawArea->loadFromCustomFormat(filePath);
            } else {
                // 为图像文件
                QImage image(filePath);
                if (!image.isNull()) {
                    m_drawArea->clearGraphics();
                    // 导入图像到场景中心
                    QPointF center = m_drawArea->scene()->sceneRect().center() - QPointF(image.width() / 2, image.height() / 2);
                    m_drawArea->importImageAt(image, center.toPoint());
                } else {
                    QMessageBox::warning(this, tr("打开失败"), tr("无法打开文件: %1").arg(filePath));
                    return;
                }
            }
            
            m_currentFilePath = filePath;
            m_isUntitled = false;
            updateWindowTitle();
            addToRecentFiles(filePath);
        } else {
            QMessageBox::warning(this, tr("文件不存在"), tr("文件 %1 不存在或已被移动").arg(filePath));
            m_recentFiles.removeOne(filePath);
            updateRecentFileActions();
        }
    }
}

// 更新最近文件动作
void MainWindow::updateRecentFileActions() {
    int numRecentFiles = qMin(m_recentFiles.size(), (int)MaxRecentFiles);
    
    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(m_recentFiles[i]).fileName());
        m_recentFileActions[i]->setText(text);
        m_recentFileActions[i]->setData(m_recentFiles[i]);
        m_recentFileActions[i]->setVisible(true);
    }
    
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j) {
        m_recentFileActions[j]->setVisible(false);
    }
    
    m_recentFileSeparator->setVisible(numRecentFiles > 0);
    m_clearRecentFilesAction->setVisible(numRecentFiles > 0);
}

// 添加到最近文件列表
void MainWindow::addToRecentFiles(const QString& filePath) {
    if (filePath.isEmpty()) return;
    
    // 如果已存在，则移除旧的条目
    m_recentFiles.removeAll(filePath);
    
    // 添加到头部
    m_recentFiles.prepend(filePath);
    
    // 限制最大数量
    while (m_recentFiles.size() > MaxRecentFiles) {
        m_recentFiles.removeLast();
    }
    
    updateRecentFileActions();
}

// 更新窗口标题
void MainWindow::updateWindowTitle() {
    if (m_isUntitled) {
        setWindowTitle(tr("无标题 - 矢量图形编辑器"));
    } else {
        setWindowTitle(tr("%1 - 矢量图形编辑器").arg(QFileInfo(m_currentFilePath).fileName()));
    }
}

// 添加onClipToolTriggered函数的实现
void MainWindow::onClipToolTriggered(bool freehandMode)
{
    // 更新状态栏
    statusBar()->showMessage(freehandMode ? tr("自由形状裁剪模式") : tr("矩形裁剪模式"));
    
    // 设置裁剪状态
    m_drawArea->setClipState(freehandMode);
}

// 在合适的位置添加createFlowchartToolbar实现
void MainWindow::createFlowchartToolbar()
{
    m_flowchartToolBar = new QToolBar("流程图工具", this);
    m_flowchartToolBar->setIconSize(QSize(24, 24));
    
    // 创建流程图工具按钮
    m_flowchartProcessAction = m_flowchartToolBar->addAction("处理框");
    m_flowchartProcessAction->setCheckable(true);
    m_flowchartProcessAction->setToolTip("绘制处理框（矩形）");
    
    m_flowchartDecisionAction = m_flowchartToolBar->addAction("判断框");
    m_flowchartDecisionAction->setCheckable(true);
    m_flowchartDecisionAction->setToolTip("绘制判断框（菱形）");
    
    m_flowchartStartEndAction = m_flowchartToolBar->addAction("开始/结束框");
    m_flowchartStartEndAction->setCheckable(true);
    m_flowchartStartEndAction->setToolTip("绘制开始/结束框（圆角矩形）");
    
    m_flowchartIOAction = m_flowchartToolBar->addAction("输入/输出框");
    m_flowchartIOAction->setCheckable(true);
    m_flowchartIOAction->setToolTip("绘制输入/输出框（平行四边形）");
    
    m_flowchartConnectorAction = m_flowchartToolBar->addAction("连接器");
    m_flowchartConnectorAction->setCheckable(true);
    m_flowchartConnectorAction->setToolTip("绘制连接线（带箭头）");
    
    // 添加自动连接工具
    m_autoConnectAction = m_flowchartToolBar->addAction("自动连接");
    m_autoConnectAction->setCheckable(true);
    m_autoConnectAction->setToolTip("自动连接模式：点击流程图元素的连接点来创建连接");
    
    // 添加分隔符
    m_flowchartToolBar->addSeparator();
    
    // 添加连接器样式选择
    QLabel* connectorTypeLabel = new QLabel("连接线类型: ");
    m_flowchartToolBar->addWidget(connectorTypeLabel);
    
    m_connectorTypeComboBox = new QComboBox();
    m_connectorTypeComboBox->addItem("直线", 0);
    m_connectorTypeComboBox->addItem("折线", 1);
    m_connectorTypeComboBox->addItem("曲线", 2);
    m_flowchartToolBar->addWidget(m_connectorTypeComboBox);
    
    // 添加箭头样式选择
    QLabel* arrowTypeLabel = new QLabel("箭头类型: ");
    m_flowchartToolBar->addWidget(arrowTypeLabel);
    
    m_arrowTypeComboBox = new QComboBox();
    m_arrowTypeComboBox->addItem("无箭头", 0);
    m_arrowTypeComboBox->addItem("单箭头", 1);
    m_arrowTypeComboBox->addItem("双箭头", 2);
    m_flowchartToolBar->addWidget(m_arrowTypeComboBox);
    
    // 添加到工具组
    toolsGroup->addAction(m_flowchartProcessAction);
    toolsGroup->addAction(m_flowchartDecisionAction);
    toolsGroup->addAction(m_flowchartStartEndAction);
    toolsGroup->addAction(m_flowchartIOAction);
    toolsGroup->addAction(m_flowchartConnectorAction);
    toolsGroup->addAction(m_autoConnectAction);
    
    // 添加到主窗口
    addToolBar(Qt::LeftToolBarArea, m_flowchartToolBar);
}