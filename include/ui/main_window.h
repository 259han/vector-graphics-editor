#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include "draw_area.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
private:
    void createToolBar();
    void createActions();

    DrawArea* m_drawArea;
    QToolBar* m_toolBar;

    QAction* m_lineAction;
    QAction* m_rectangleAction;
    QAction* m_circleAction;
    QAction* m_triangleAction;
    QAction* m_flowchartAction;
    QAction* m_editAction;
    QAction* m_ellipseAction;
    QAction* m_bezierAction;
    QAction* m_fillColorAction;
    QAction* m_loadImageAction;
};

#endif // MAIN_WINDOW_H