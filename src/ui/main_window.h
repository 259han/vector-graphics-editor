#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QSlider>
#include <QLabel>
#include <QSpinBox>
#include <QActionGroup>
#include <QPushButton>
#include <QComboBox>
#include "draw_area.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onDrawActionTriggered(QAction* action);
    void onTransformActionTriggered(QAction* action);
    void onLayerActionTriggered(QAction* action);
    void onEditActionTriggered(QAction* action);
    void onFillColorTriggered();
    void onFillToolTriggered();
    void onSelectFillColor();
    
    void onGridToggled(bool enabled);
    void onGridSizeChanged(int size);
    void onSnapToGridToggled(bool enabled);

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createFillSettingsDialog();
    void createToolOptions();

    DrawArea* m_drawArea;

    // 工具栏
    QToolBar* m_drawToolBar;
    QToolBar* m_transformToolBar;
    QToolBar* m_layerToolBar;
    QToolBar* m_fillToolBar;
    
    // 绘图和选择工具
    QAction* m_selectAction;
    QAction* m_lineAction;
    QAction* m_rectAction;
    QAction* m_ellipseAction;
    QAction* m_bezierAction;
    QAction* m_fillToolAction;
    
    // 文件操作
    QAction* m_importImageAction;
    QAction* m_saveImageAction;
    QAction* m_clearAction;

    // 变换工具
    QAction* m_rotateAction;
    QAction* m_scaleAction;
    QAction* m_deleteAction;

    // 图层工具
    QAction* m_bringToFrontAction;
    QAction* m_sendToBackAction;
    QAction* m_bringForwardAction;
    QAction* m_sendBackwardAction;

    // 编辑工具
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_cutAction;
    
    // 填充设置控件
    QWidget* m_fillSettingsWidget;
    QPushButton* m_colorButton;
    QColor m_currentFillColor;

    // 工具组
    QActionGroup* toolsGroup;
    
    // 其他动作
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportAction;
    
    QAction* m_gridAction;
    QAction* m_snapAction;
    
    QComboBox* m_gridSizeComboBox;

    QVector<QAction*> recentFileActions;
};

#endif // MAIN_WINDOW_H