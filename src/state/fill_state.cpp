#include "fill_state.h"
#include "../ui/draw_area.h"
#include "../utils/graphics_utils.h"
#include "../utils/logger.h"
#include "../command/fill_command.h"
#include "../command/command_manager.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QDebug>
#include <QGraphicsScene>
#include <QApplication>
#include <QCursor>
#include <QGraphicsPixmapItem>
#include <QMainWindow>
#include <QStatusBar>
#include <QStack>

FillState::FillState(const QColor& fillColor)
    : m_fillColor(fillColor)
{
    Logger::debug(QString("FillState: 创建填充状态，颜色: %1").arg(fillColor.name(QColor::HexArgb)));
}

void FillState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    m_isPressed = true;
    m_currentPoint = scenePos;
    
    // 设置等待光标，表示填充操作正在进行
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    Logger::debug(QString("FillState: 开始填充 - 位置: (%1, %2)").arg(scenePos.x()).arg(scenePos.y()));
}

void FillState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    // 右键点击切换回编辑模式
    exitCurrentState(drawArea);
    Logger::debug("填充工具: 右键点击，切换回编辑模式");
}

void FillState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 获取鼠标点击位置
        QPointF scenePos = getScenePos(drawArea, event);
        handleLeftMousePress(drawArea, scenePos);
        
        // 事件已处理
        event->accept();
    } else if (event->button() == Qt::RightButton) {
        QPointF scenePos = getScenePos(drawArea, event);
        handleRightMousePress(drawArea, scenePos);
        
        // 事件已处理
        event->accept();
    }
}

void FillState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // 如果已经按下，现在释放，则执行填充操作
        if (m_isPressed) {
            m_isPressed = false;
            
            // 保存当前点
            m_currentPoint = getScenePos(drawArea, event);
            
            // 执行填充操作
            finishOperation(drawArea);
            
            // 事件已处理
            event->accept();
        }
    }
}

void FillState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) {
    m_lastPoint = event->pos();
    
    // 更新状态信息
    updateStatusMessage(drawArea, "填充工具: 点击要填充的区域");
    
    // 确保使用填充工具的光标
    setCursor(drawArea, Qt::PointingHandCursor);
}

void FillState::paintEvent(DrawArea* drawArea, QPainter* painter) {
    // 无需额外绘制
}

void FillState::keyPressEvent(DrawArea* drawArea, QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        exitCurrentState(drawArea);
    }
}

// 执行填充操作
void FillState::fillRegion(DrawArea* drawArea, const QPointF& startPoint) {
    // 保存填充位置
    m_currentPoint = startPoint;
    
    // 调用完成操作方法，该方法将创建并执行填充命令
    finishOperation(drawArea);
}

void FillState::wheelEvent(DrawArea* drawArea, QWheelEvent* event)
{
    // 使用基类的缩放和平移处理
    if (handleZoomAndPan(drawArea, event)) {
        return;
    }
    
    // 如果基类没有处理，则传递给父类的默认处理
    event->ignore();
}

void FillState::onEnterState(DrawArea* drawArea)
{
    logInfo("FillState: 进入填充状态");
    
    // 设置填充工具光标
    setCursor(drawArea, QCursor(Qt::PointingHandCursor));
    updateStatusMessage(drawArea, QString("填充工具：当前颜色 %1，点击区域进行填充").arg(m_fillColor.name()));
}

void FillState::onExitState(DrawArea* drawArea)
{
    logInfo("FillState: 退出填充状态");
    
    // 重置鼠标光标
    resetCursor(drawArea);
}

void FillState::handleMiddleMousePress(DrawArea* drawArea, QPointF scenePos)
{
    logDebug(QString("FillState: 中键点击，位置: (%1, %2)").arg(scenePos.x()).arg(scenePos.y()));
    
    // 中键点击用于在填充过程中临时切换到视图平移模式
    if (drawArea) {
        drawArea->setDragMode(QGraphicsView::ScrollHandDrag);
        setCursor(drawArea, Qt::ClosedHandCursor);
    }
}

void FillState::finishOperation(DrawArea* drawArea)
{
    if (!drawArea) {
        Logger::error("FillState::finishOperation: drawArea is null");
        return;
    }

    // 获取填充点和颜色
    QPointF fillPosition = m_currentPoint;
    QColor fillColor = m_fillColor;

    // 创建填充命令
    FillCommand* command = new FillCommand(drawArea, fillPosition, fillColor);
    
    // 执行命令
    CommandManager::getInstance().executeCommand(command);
    
    Logger::info(QString("FillState: 执行填充命令 - 位置: (%1, %2), 颜色: %3")
                 .arg(fillPosition.x()).arg(fillPosition.y())
                 .arg(fillColor.name()));
    
    // 恢复光标
    QApplication::restoreOverrideCursor();
}
