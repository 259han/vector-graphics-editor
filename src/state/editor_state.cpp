#include "editor_state.h"
#include "../ui/draw_area.h"
#include "../utils/logger.h"
#include "../command/command_manager.h"
#include <QMainWindow>
#include <QStatusBar>
#include <QGraphicsView>
#include <QApplication>
#include <QToolTip>

EditorState::EditorState() : m_isPressed(false) {
    // 构造函数简单初始化基本属性
    logDebug("创建新的编辑器状态");
}

void EditorState::updateStatusMessage(DrawArea* drawArea, const QString& message) {
    if (!drawArea) return;
    
    QWidget* topLevelWidget = drawArea->window();
    if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(topLevelWidget)) {
        mainWindow->statusBar()->showMessage(message);
    }
}

void EditorState::setCursor(DrawArea* drawArea, const QCursor& cursor) {
    if (drawArea) {
        drawArea->setCursor(cursor);
    }
}

void EditorState::resetCursor(DrawArea* drawArea) {
    if (drawArea) {
        drawArea->unsetCursor();
    }
}

QPointF EditorState::getScenePos(DrawArea* drawArea, QMouseEvent* event) {
    if (!drawArea || !event) return QPointF();
    
    return drawArea->mapToScene(event->pos());
}

QPointF EditorState::getSnapToGridPoint(DrawArea* drawArea, const QPointF& scenePos) {
    if (!drawArea) return scenePos;
    
    // 获取网格大小
    int gridSize = drawArea->getGridSize();
    
    // 如果网格功能未启用或网格大小为0，直接返回原始位置
    if (!drawArea->isGridEnabled() || gridSize <= 0) {
        return scenePos;
    }
    
    // 计算对齐到网格的位置
    qreal x = qRound(scenePos.x() / gridSize) * gridSize;
    qreal y = qRound(scenePos.y() / gridSize) * gridSize;
    
    return QPointF(x, y);
}

void EditorState::exitCurrentState(DrawArea* drawArea) {
    if (drawArea) {
        // 记录开始切换状态
        Logger::debug("EditorState::exitCurrentState: 开始切换状态");
        
        // 调用onExitState通知当前状态即将结束
        try {
            Logger::debug("EditorState::exitCurrentState: 正在调用当前状态的onExitState");
            onExitState(drawArea);
            Logger::debug("EditorState::exitCurrentState: 当前状态的onExitState调用完成");
        } catch (const std::exception& e) {
            Logger::error(QString("EditorState::exitCurrentState: onExitState发生异常: %1").arg(e.what()));
        } catch (...) {
            Logger::error("EditorState::exitCurrentState: onExitState发生未知异常");
        }
        
        // 确保UI能响应
        QApplication::processEvents();
        
        // 切换到编辑状态
        Logger::debug("EditorState::exitCurrentState: 准备设置编辑状态");
        drawArea->setEditState();
        Logger::debug("EditorState::exitCurrentState: 编辑状态设置完成");
        
        // 再次确保UI能响应
        QApplication::processEvents();
    } else {
        Logger::warning("EditorState::exitCurrentState: DrawArea为空，无法切换状态");
    }
}

void EditorState::logDebug(const QString& message) {
    // 使用Logger类记录调试信息
    Logger::debug(message);
}

void EditorState::logInfo(const QString& message) {
    // 使用Logger类记录一般信息
    Logger::info(message);
}

void EditorState::logWarning(const QString& message) {
    // 使用Logger类记录警告信息
    Logger::warning(message);
}

void EditorState::logError(const QString& message) {
    // 使用Logger类记录错误信息
    Logger::error(message);
}

bool EditorState::checkRightClickToEdit(DrawArea* drawArea, QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        exitCurrentState(drawArea);
        logDebug("右键点击，切换到编辑模式");
        event->accept();
        return true;
    }
    return false;
}

bool EditorState::handleCommonKeyboardShortcuts(DrawArea* drawArea, QKeyEvent* event) {
    if (!drawArea || !event) return false;
    
    // 处理 Escape 键 - 切换到编辑状态
    if (event->key() == Qt::Key_Escape) {
        exitCurrentState(drawArea);
        logDebug("按下Escape键，切换到编辑模式");
        event->accept();
        return true;
    }
    
    // 处理 Ctrl+Z - 撤销
    if (event->key() == Qt::Key_Z && event->modifiers() == Qt::ControlModifier) {
        CommandManager::getInstance().undo();
        logDebug("按下Ctrl+Z，执行撤销操作");
        event->accept();
        return true;
    }
    
    // 处理 Ctrl+Y - 重做
    if (event->key() == Qt::Key_Y && event->modifiers() == Qt::ControlModifier) {
        CommandManager::getInstance().redo();
        logDebug("按下Ctrl+Y，执行重做操作");
        event->accept();
        return true;
    }
    
    // 处理 Ctrl+A - 全选
    if (event->key() == Qt::Key_A && event->modifiers() == Qt::ControlModifier) {
        drawArea->selectAllGraphics();
        logDebug("按下Ctrl+A，执行全选操作");
        event->accept();
        return true;
    }
    
    // 其他快捷键可以根据需要添加
    
    return false;  // 没有处理任何快捷键
}

bool EditorState::handleZoomAndPan(DrawArea* drawArea, QWheelEvent* event) {
    if (!drawArea || !event) return false;
    
    // Ctrl+滚轮缩放
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            // 放大
            drawArea->scale(1.2, 1.2);
        } else {
            // 缩小
            drawArea->scale(1.0 / 1.2, 1.0 / 1.2);
        }
        logDebug("执行缩放操作");
        event->accept();
        return true;
    }
    
    return false;  // 没有处理缩放或平移
}

void EditorState::setToolTip(DrawArea* drawArea, const QString& tip) {
    if (drawArea) {
        QToolTip::showText(QCursor::pos(), tip);
    }
}

// 纯虚基类无需实现 