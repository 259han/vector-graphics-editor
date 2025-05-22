#include "clip_state.h"
#include "../ui/draw_area.h"
#include "../utils/logger.h"
#include "../command/clip_command.h"
#include "../command/command_manager.h"
#include "../core/graphic_item.h"
#include <QGraphicsPathItem>
#include <QApplication>
#include <QMessageBox>
#include <QPen>
#include <QBrush>

ClipState::ClipState()
{
    logInfo("ClipState::ClipState: 创建裁剪状态");
}

void ClipState::onEnterState(DrawArea* drawArea)
{
    logInfo("ClipState::onEnterState: 进入裁剪状态");
    
    // 检查是否有选中的图形项
    m_selectedItems = drawArea->getSelectedItems();
    if (m_selectedItems.isEmpty()) {
        // 如果没有选中的图形项，显示提示消息
        updateStatusMessage(drawArea, "请先选择要裁剪的图形");
        QMessageBox::information(drawArea, "裁剪工具", "请先选择要裁剪的图形，然后再进入裁剪模式。");
        // 返回到编辑状态
        drawArea->setEditState();
        return;
    }
    
    // 设置鼠标光标
    setCursor(drawArea, Qt::CrossCursor);
    
    // 显示状态消息
    updateStatusMessage(drawArea, "裁剪模式：点击并拖动鼠标定义裁剪区域，按ESC取消");
    
    // 重置裁剪状态
    m_isClipping = false;
    m_freehandPoints.clear();
    
    // 保持选中的图形项
    drawArea->clearSelection();
}

void ClipState::onExitState(DrawArea* drawArea)
{
    logInfo("ClipState::onExitState: 退出裁剪状态");
    
    // 清除临时图形项
    clearTemporaryItems(drawArea);
    
    // 重置光标
    resetCursor(drawArea);
}

void ClipState::mousePressEvent(DrawArea* drawArea, QMouseEvent* event)
{
    // 获取场景坐标
    QPointF scenePos = getScenePos(drawArea, event);
    
    // 处理左键点击
    if (event->button() == Qt::LeftButton) {
        handleLeftMousePress(drawArea, scenePos);
    }
    // 处理右键点击
    else if (event->button() == Qt::RightButton) {
        handleRightMousePress(drawArea, scenePos);
    }
}

void ClipState::handleLeftMousePress(DrawArea* drawArea, QPointF scenePos)
{
    // 如果已经在裁剪状态，直接返回
    if (m_isClipping) {
        return;
    }
    
    // 开始裁剪操作
    logDebug(QString("ClipState::handleLeftMousePress: 开始裁剪操作，位置: (%1, %2)")
            .arg(scenePos.x())
            .arg(scenePos.y()));
    
    // 记录起始点
    m_startPoint = scenePos;
    m_currentPoint = scenePos;
    
    // 创建临时图形项表示裁剪区域
    if (m_clipAreaMode == RectangleClip) {
        m_clipRectItem = new QGraphicsRectItem(QRectF(m_startPoint, m_startPoint));
        
        // 设置样式
        QPen pen(m_clipAreaOutlineColor);
        pen.setStyle(Qt::DashLine);
        QBrush brush(m_clipAreaFillColor);
        
        m_clipRectItem->setPen(pen);
        m_clipRectItem->setBrush(brush);
        
        // 添加到场景
        drawArea->scene()->addItem(m_clipRectItem);
    }
    else if (m_clipAreaMode == FreehandClip) {
        // 创建路径
        QPainterPath path;
        path.moveTo(m_startPoint);
        
        m_freehandPoints.clear();
        m_freehandPoints.push_back(m_startPoint);
        
        // 创建路径图形项
        m_clipPathItem = new QGraphicsPathItem(path);
        
        // 设置样式
        QPen pen(m_clipAreaOutlineColor);
        QBrush brush(m_clipAreaFillColor);
        
        m_clipPathItem->setPen(pen);
        m_clipPathItem->setBrush(brush);
        
        // 添加到场景
        drawArea->scene()->addItem(m_clipPathItem);
    }
    
    m_isClipping = true;
}

void ClipState::handleRightMousePress(DrawArea* drawArea, QPointF scenePos)
{
    // 右键点击取消裁剪操作
    cancelClip(drawArea);
}

void ClipState::mouseReleaseEvent(DrawArea* drawArea, QMouseEvent* event)
{
    // 只处理左键释放
    if (event->button() != Qt::LeftButton || !m_isClipping) {
        return;
    }
    
    // 完成裁剪操作
    finishClip(drawArea);
}

void ClipState::mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event)
{
    // 更新当前点
    m_currentPoint = getScenePos(drawArea, event);
    
    // 更新裁剪区域预览
    if (m_isClipping) {
        updateClipPreview(drawArea);
    }
}

void ClipState::updateClipPreview(DrawArea* drawArea)
{
    if (m_clipAreaMode == RectangleClip && m_clipRectItem) {
        // 更新矩形裁剪区域
        QRectF rect = QRectF(m_startPoint, m_currentPoint).normalized();
        m_clipRectItem->setRect(rect);
    }
    else if (m_clipAreaMode == FreehandClip && m_clipPathItem) {
        // 更新自由形状裁剪区域
        
        // 控制点采样频率，避免添加过多点，但也不能太少
        // 减小采样间隔，提高精度
        if (m_freehandPoints.empty() || 
            (m_currentPoint - m_freehandPoints.back()).manhattanLength() > 5) {  // 从10减少到5
            m_freehandPoints.push_back(m_currentPoint);
            
            // 创建路径
            QPainterPath path;
            if (!m_freehandPoints.empty()) {
                path.moveTo(m_freehandPoints[0]);
                for (size_t i = 1; i < m_freehandPoints.size(); ++i) {
                    path.lineTo(m_freehandPoints[i]);
                }
                // 封闭路径
                if (m_freehandPoints.size() > 2) {
                    // 添加一条从当前点到起点的虚线，表示路径会被闭合
                    path.lineTo(m_freehandPoints[0]);
                }
            }
            
            // 更新路径
            m_clipPathItem->setPath(path);
            
            // 更新画笔样式，使用虚线表示未完成的闭合路径
            QPen pen(m_clipAreaOutlineColor);
            pen.setStyle(Qt::DashLine);
            m_clipPathItem->setPen(pen);
        }
    }
}

void ClipState::finishClip(DrawArea* drawArea)
{
    if (!m_isClipping) {
        return;
    }
    
    // 获取裁剪路径
    QPainterPath clipPath;
    
    if (m_clipAreaMode == RectangleClip && m_clipRectItem) {
        QRectF rect = m_clipRectItem->rect();
        // 确保矩形有足够的大小
        if (rect.width() < 5 || rect.height() < 5) {
            logWarning("ClipState::finishClip: 裁剪区域太小，取消裁剪");
            cancelClip(drawArea);
            return;
        }
        clipPath.addRect(rect);
    }
    else if (m_clipAreaMode == FreehandClip && m_clipPathItem) {
        // 确保至少有3个点以形成一个有效的多边形
        if (m_freehandPoints.size() < 3) {
            logWarning("ClipState::finishClip: 自由形状裁剪路径点数不足，取消裁剪");
            cancelClip(drawArea);
            return;
        }
        
        // 自由形状裁剪可能会比较复杂，进行简化处理
        // 如果点数过多，进行适当简化，保留关键点
        if (m_freehandPoints.size() > 100) {
            std::vector<QPointF> simplifiedPoints;
            simplifiedPoints.reserve(100);
            
            // 简单的降采样，保留关键拐点
            double skipFactor = static_cast<double>(m_freehandPoints.size()) / 100.0;
            for (size_t i = 0; i < m_freehandPoints.size(); i += std::max(1.0, skipFactor)) {
                size_t idx = std::min(i, m_freehandPoints.size() - 1);
                simplifiedPoints.push_back(m_freehandPoints[idx]);
            }
            
            // 确保包含起点和终点
            if (!simplifiedPoints.empty() && simplifiedPoints.back() != m_freehandPoints.back()) {
                simplifiedPoints.push_back(m_freehandPoints.back());
            }
            
            m_freehandPoints = simplifiedPoints;
            logInfo(QString("ClipState::finishClip: 路径点数过多，已简化为 %1 个点").arg(m_freehandPoints.size()));
        }
        
        // 创建平滑的自由形状路径
        clipPath = QPainterPath();
        clipPath.moveTo(m_freehandPoints[0]);
        
        // 添加所有点，并闭合路径
        for (size_t i = 1; i < m_freehandPoints.size(); ++i) {
            clipPath.lineTo(m_freehandPoints[i]);
        }
        
        // 闭合路径（连接最后一个点和第一个点）
        clipPath.closeSubpath();
        
        // 更新预览
        m_clipPathItem->setPath(clipPath);
        
        // 更新画笔样式，使用实线表示完成的闭合路径
        QPen pen(m_clipAreaOutlineColor);
        pen.setStyle(Qt::SolidLine);
        m_clipPathItem->setPen(pen);
        
        // 检查路径是否有效
        if (clipPath.isEmpty()) {
            logWarning("ClipState::finishClip: 自由形状裁剪路径无效，取消裁剪");
            cancelClip(drawArea);
            return;
        }
        
        logInfo(QString("ClipState::finishClip: 自由形状裁剪路径创建完成，顶点数: %1")
               .arg(m_freehandPoints.size()));
    }
    
    // 获取裁剪区域矩形（用于日志记录）
    QRectF clipRect = clipPath.boundingRect();
    logInfo(QString("ClipState::finishClip: 裁剪区域矩形 (%1,%2,%3,%4)")
             .arg(clipRect.left()).arg(clipRect.top())
             .arg(clipRect.width()).arg(clipRect.height()));
    
    // 处理每个选中的图形项
    CommandManager& cmdManager = CommandManager::getInstance();
    bool anythingClipped = false;
    
    // 开始命令分组
    cmdManager.beginCommandGroup();
    
    for (QGraphicsItem* item : m_selectedItems) {
        // 尝试将项目转换为GraphicItem
        if (GraphicItem* graphicItem = dynamic_cast<GraphicItem*>(item)) {
            // 创建裁剪命令
            ClipCommand* cmd = new ClipCommand(drawArea->scene(), graphicItem, clipPath);
            
            // 添加到命令组
            cmdManager.addCommandToGroup(cmd);
            anythingClipped = true;
        }
    }
    
    // 提交命令组
    if (anythingClipped) {
        cmdManager.commitCommandGroup();
        logInfo("ClipState::finishClip: 裁剪操作完成");
    } else {
        cmdManager.clear(); // 清除命令组
        logWarning("ClipState::finishClip: 没有图形项被裁剪");
    }
    
    // 清除临时图形项
    clearTemporaryItems(drawArea);
    
    // 重置裁剪状态
    m_isClipping = false;
    
    // 更新状态消息
    updateStatusMessage(drawArea, anythingClipped ? "裁剪完成" : "裁剪失败，没有图形项被裁剪");
    
    // 返回到编辑状态
    drawArea->setEditState();
}

void ClipState::cancelClip(DrawArea* drawArea)
{
    logInfo("ClipState::cancelClip: 取消裁剪操作");
    
    // 清除临时图形项
    clearTemporaryItems(drawArea);
    
    // 重置裁剪状态
    m_isClipping = false;
    m_freehandPoints.clear();
    
    // 更新状态消息
    updateStatusMessage(drawArea, "裁剪操作已取消");
    
    // 返回到编辑状态
    drawArea->setEditState();
}

void ClipState::clearTemporaryItems(DrawArea* drawArea)
{
    // 清除矩形裁剪区域
    if (m_clipRectItem) {
        drawArea->scene()->removeItem(m_clipRectItem);
        delete m_clipRectItem;
        m_clipRectItem = nullptr;
    }
    
    // 清除路径裁剪区域
    if (m_clipPathItem) {
        drawArea->scene()->removeItem(m_clipPathItem);
        delete m_clipPathItem;
        m_clipPathItem = nullptr;
    }
}

void ClipState::keyPressEvent(DrawArea* drawArea, QKeyEvent* event)
{
    // 处理ESC键取消裁剪
    if (event->key() == Qt::Key_Escape) {
        cancelClip(drawArea);
        return;
    }
    
    // 处理通用键盘快捷键
    if (handleCommonKeyboardShortcuts(drawArea, event)) {
        return;
    }
}

void ClipState::paintEvent(DrawArea* drawArea, QPainter* painter)
{
    // 不需要额外绘制
}

void ClipState::setClipAreaMode(ClipAreaMode mode)
{
    m_clipAreaMode = mode;
    logInfo(QString("ClipState::setClipAreaMode: 设置裁剪模式为 %1")
            .arg(mode == RectangleClip ? "矩形裁剪" : "自由形状裁剪"));
} 