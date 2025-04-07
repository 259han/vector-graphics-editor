#include "ui/image_resizer.h"
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QCursor>
#include <QApplication>

ImageResizer::ImageResizer(QGraphicsItem* target)
    : QGraphicsObject(),
      m_target(target),
      m_currentHandle(-1),
      m_startPos(),
      m_originalCenter(),
      m_originalSize(),
      m_originalRotation(0),
      m_startAngle(0)
{
    // 确保控制点在其他项上层，使用非常高的z值
    setZValue(9999); 
    
    // 设置本体的flags
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setAcceptHoverEvents(true);
    
    // 不要作为target的子项，这会导致变换问题
    // 而是直接添加到同一个场景中
    QGraphicsScene* targetScene = target->scene();
    if (targetScene) {
        // 先添加到场景，再进行任何其他操作
        targetScene->addItem(this);
        
        // 确保目标项的标志设置正确
        if (!target->flags().testFlag(QGraphicsItem::ItemIsMovable)) {
            target->setFlag(QGraphicsItem::ItemIsMovable, true);
            qDebug() << "ImageResizer: Forcing ItemIsMovable flag on target";
        }
        
        if (!target->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
            target->setFlag(QGraphicsItem::ItemIsSelectable, true);
            qDebug() << "ImageResizer: Forcing ItemIsSelectable flag on target";
        }
        
        // 强制启用target的选择状态
        if (!target->isSelected()) {
            target->setSelected(true);
            qDebug() << "ImageResizer: Forcing selection state on target";
        }
        
        // 创建控制点
        createHandles();
        
        // 更新控制点位置以匹配target
        updateHandles();
        
        // 强制所有控制点可见
        for (int i = 0; i < m_handles.size(); ++i) {
            m_handles[i]->setVisible(true);
            m_handles[i]->setZValue(10000); // 确保控制点在最上层
        }
        
        if (m_rotateLine) {
            m_rotateLine->setVisible(true);
            m_rotateLine->setZValue(10000);
        }
        
        if (m_rotateHandle) {
            m_rotateHandle->setVisible(true);
            m_rotateHandle->setZValue(10001); // 旋转控制点在最上层
        }
        
        // 在添加到场景后，安装事件过滤器，以便接收目标项的事件
        target->installSceneEventFilter(this);
        qDebug() << "ImageResizer: Installed event filter on target";
        
        // 为新创建的控制点安装事件过滤器
        for (int i = 0; i < m_handles.size(); ++i) {
            QGraphicsRectItem* handle = m_handles[i];
            handle->installSceneEventFilter(this);
            qDebug() << "ImageResizer: Installed event filter on handle" << i;
        }
        
        if (m_rotateHandle) {
            m_rotateHandle->installSceneEventFilter(this);
            qDebug() << "ImageResizer: Installed event filter on rotate handle";
        }
    } else {
        qWarning() << "ImageResizer: Target item is not in a scene!";
    }
    
    // 默认显示控制点
    setVisible(true);
    
    qDebug() << "ImageResizer created for target item at:" << target->pos();
}

ImageResizer::~ImageResizer()
{
    // 清理控制点
    qDeleteAll(m_handles);
    m_handles.clear();
    
    // 清理旋转控制线和控制点
    delete m_rotateLine;
    delete m_rotateHandle;
}

QRectF ImageResizer::boundingRect() const
{
    // 边界矩形需要包含目标项和所有控制点
    QRectF rect = m_target->boundingRect();
    rect = m_target->mapToScene(rect).boundingRect();
    
    // 扩展以包含旋转控制点
    if (m_rotateHandle) {
        rect = rect.united(m_rotateHandle->mapToScene(m_rotateHandle->boundingRect()).boundingRect());
    }
    
    // 添加一些边距
    return rect.adjusted(-20, -20, 20, 20);
}

void ImageResizer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    // 无论是否选中目标项，都显示边界框，这样更容易找到图像
    if (m_target) {
        // 绘制目标项的边界框
        QRectF rect = m_target->boundingRect();
        QTransform targetTransform = m_target->sceneTransform();
        
        painter->save();
        painter->setTransform(targetTransform, true);
        
        // 使用蓝色虚线绘制边框
        QPen framePen(QColor(0, 120, 250), 1.5, Qt::DashLine);
        painter->setPen(framePen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
        
        painter->restore();
        
        // 确保调整器和控制点可见
        setVisible(true);
        
        // 强制所有控制点可见
        for (int i = 0; i < m_handles.size(); ++i) {
            if (m_handles[i]) {
                m_handles[i]->setVisible(true);
            }
        }
        
        if (m_rotateLine) {
            m_rotateLine->setVisible(true);
        }
        
        if (m_rotateHandle) {
            m_rotateHandle->setVisible(true);
        }
    }
}

bool ImageResizer::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    // 处理控制点的事件
    for (int i = 0; i < m_handles.size(); ++i) {
        if (watched == m_handles[i]) {
            QGraphicsSceneMouseEvent *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
            if (!mouseEvent) {
                return false;
            }
            
            switch (event->type()) {
            case QEvent::GraphicsSceneMousePress:
                if (mouseEvent->button() == Qt::LeftButton) {
                    m_currentHandle = i;
                    m_startPos = mouseEvent->scenePos();
                    
                    // 记录原始大小和位置
                    m_originalSize = QSizeF(m_target->boundingRect().width(), m_target->boundingRect().height());
                    m_originalCenter = m_target->mapToScene(m_target->boundingRect().center());
                    
                    qDebug() << "ImageResizer: Handle" << i << "pressed at" << m_startPos;
                    return true; // 消费事件
                }
                break;
                
            case QEvent::GraphicsSceneMouseMove:
                if (m_currentHandle == i) {
                    QPointF pos = mouseEvent->scenePos();
                    resize(pos);
                    qDebug() << "ImageResizer: Moving handle" << i << "to" << pos;
                    return true; // 消费事件
                }
                break;
                
            case QEvent::GraphicsSceneMouseRelease:
                if (m_currentHandle == i && mouseEvent->button() == Qt::LeftButton) {
                    m_currentHandle = -1;
                    qDebug() << "ImageResizer: Handle" << i << "released";
                    return true; // 消费事件
                }
                break;
                
            case QEvent::GraphicsSceneHoverEnter:
                setCursor(getCursorForHandle(i));
                qDebug() << "ImageResizer: Mouse entered handle" << i;
                return true; // 消费事件
                
            case QEvent::GraphicsSceneHoverLeave:
                unsetCursor();
                qDebug() << "ImageResizer: Mouse left handle" << i;
                return true; // 消费事件
                
            default:
                break;
            }
        }
    }
    
    // 处理旋转控制点的事件
    if (watched == m_rotateHandle) {
        QGraphicsSceneMouseEvent *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
        if (!mouseEvent) {
            return false;
        }
        
        switch (event->type()) {
        case QEvent::GraphicsSceneMousePress:
            if (mouseEvent->button() == Qt::LeftButton) {
                m_currentHandle = Handle::Rotate;
                m_startPos = mouseEvent->scenePos();
                
                // 记录原始位置和旋转角度
                m_originalCenter = m_target->mapToScene(m_target->boundingRect().center());
                m_originalRotation = m_target->rotation();
                
                // 计算初始角度
                QLineF line(m_originalCenter, m_startPos);
                m_startAngle = line.angle(); // Qt中角度顺时针增长，与标准坐标系相反
                
                qDebug() << "ImageResizer: Rotate handle pressed at" << m_startPos
                         << ", originalRotation:" << m_originalRotation
                         << ", startAngle:" << m_startAngle;
                return true; // 消费事件
            }
            break;
            
        case QEvent::GraphicsSceneMouseMove:
            if (m_currentHandle == Handle::Rotate) {
                QPointF pos = mouseEvent->scenePos();
                rotate(pos);
                qDebug() << "ImageResizer: Moving rotate handle to" << pos;
                return true; // 消费事件
            }
            break;
            
        case QEvent::GraphicsSceneMouseRelease:
            if (m_currentHandle == Handle::Rotate && mouseEvent->button() == Qt::LeftButton) {
                m_currentHandle = -1;
                qDebug() << "ImageResizer: Rotate handle released";
                return true; // 消费事件
            }
            break;
            
        case QEvent::GraphicsSceneHoverEnter:
            setCursor(Qt::CrossCursor); // 旋转控制点使用十字光标
            qDebug() << "ImageResizer: Mouse entered rotate handle";
            return true; // 消费事件
            
        case QEvent::GraphicsSceneHoverLeave:
            unsetCursor();
            qDebug() << "ImageResizer: Mouse left rotate handle";
            return true; // 消费事件
            
        default:
            break;
        }
    }
    
    // 处理目标项的事件
    if (watched == m_target) {
        QGraphicsSceneMouseEvent *mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
        
        switch (event->type()) {
        case QEvent::GraphicsSceneMousePress:
            if (mouseEvent && mouseEvent->button() == Qt::LeftButton) {
                // 目标项被点击时，确保调整器可见
                setVisible(true);
                updateHandles();
                qDebug() << "ImageResizer: Target item pressed, updating handles";
            }
            break;
            
        case QEvent::GraphicsSceneMouseMove:
            if (mouseEvent && m_currentHandle == -1) {
                // 用户正在移动目标项，更新控制点位置
                updateHandles();
                qDebug() << "ImageResizer: Target item moved, updating handles";
            }
            break;
            
        default:
            break;
        }
    }
    
    // 对于目标项，不消费事件，让正常的移动行为继续
    return false;
}

bool ImageResizer::sceneEvent(QEvent *event)
{
    // 处理自己接收到的事件
    switch (event->type()) {
    case QEvent::GraphicsSceneMouseMove:
        // 当鼠标在调整器上移动时，确保更新控制点位置
        if (m_target && m_currentHandle == -1) {
            updateHandles();
            qDebug() << "ImageResizer: Mouse moved over resizer, updating handles";
        }
        break;
    default:
        break;
    }
    
    return QGraphicsObject::sceneEvent(event);
}

void ImageResizer::createHandles()
{
    // 创建8个调整大小的控制点
    for (int i = 0; i < 8; ++i) {
        QGraphicsRectItem* handle = new QGraphicsRectItem(this);
        
        // 设置控制点的外观
        handle->setRect(-4, -4, 8, 8);
        handle->setPen(QPen(Qt::blue, 1));
        handle->setBrush(QBrush(Qt::white));
        
        // 设置控制点的标志和属性
        handle->setFlag(QGraphicsItem::ItemIsMovable, false);
        handle->setFlag(QGraphicsItem::ItemIsSelectable, false);
        handle->setAcceptHoverEvents(true);
        
        // 存储控制点
        m_handles.append(handle);
    }
    
    // 创建旋转控制线和控制点
    m_rotateLine = new QGraphicsLineItem(this);
    m_rotateLine->setPen(QPen(Qt::blue, 1, Qt::DashLine));
    
    m_rotateHandle = new QGraphicsEllipseItem(this);
    m_rotateHandle->setRect(-5, -5, 10, 10);
    m_rotateHandle->setPen(QPen(Qt::red, 1));
    m_rotateHandle->setBrush(QBrush(Qt::white));
    m_rotateHandle->setFlag(QGraphicsItem::ItemIsMovable, false);
    m_rotateHandle->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_rotateHandle->setAcceptHoverEvents(true);
}

void ImageResizer::updateHandles()
{
    if (!m_target) {
        qWarning() << "ImageResizer::updateHandles: No target item!";
        return;
    }
    
    // 获取目标项的边界矩形（在场景坐标系中）
    QRectF rect = m_target->boundingRect();
    
    // 将目标矩形转换为场景坐标
    QPolygonF poly = m_target->mapToScene(rect);
    QRectF sceneRect = poly.boundingRect();
    
    // 设置8个控制点的位置
    if (m_handles.size() == 8) {
        // 左上角
        m_handles[TopLeft]->setPos(sceneRect.left(), sceneRect.top());
        
        // 顶部中心
        m_handles[Top]->setPos(sceneRect.left() + sceneRect.width() / 2, sceneRect.top());
        
        // 右上角
        m_handles[TopRight]->setPos(sceneRect.right(), sceneRect.top());
        
        // 右侧中心
        m_handles[Right]->setPos(sceneRect.right(), sceneRect.top() + sceneRect.height() / 2);
        
        // 右下角
        m_handles[BottomRight]->setPos(sceneRect.right(), sceneRect.bottom());
        
        // 底部中心
        m_handles[Bottom]->setPos(sceneRect.left() + sceneRect.width() / 2, sceneRect.bottom());
        
        // 左下角
        m_handles[BottomLeft]->setPos(sceneRect.left(), sceneRect.bottom());
        
        // 左侧中心
        m_handles[Left]->setPos(sceneRect.left(), sceneRect.top() + sceneRect.height() / 2);
    } else {
        qWarning() << "ImageResizer::updateHandles: Incorrect number of handles:" << m_handles.size();
    }
    
    // 更新旋转控制点的位置
    if (m_rotateLine && m_rotateHandle) {
        QPointF center = m_target->mapToScene(m_target->boundingRect().center());
        QPointF topMiddle = QPointF(center.x(), sceneRect.top() - 40); // 旋转控制点在顶部上方
        
        m_rotateLine->setLine(QLineF(center, topMiddle));
        m_rotateHandle->setPos(topMiddle);
    }
    
    // 更新这个调整器项的边界矩形
    prepareGeometryChange();
}

Qt::CursorShape ImageResizer::getCursorForHandle(int handle)
{
    switch (handle) {
    case TopLeft:
    case BottomRight:
        return Qt::SizeFDiagCursor;
        
    case TopRight:
    case BottomLeft:
        return Qt::SizeBDiagCursor;
        
    case Top:
    case Bottom:
        return Qt::SizeVerCursor;
        
    case Left:
    case Right:
        return Qt::SizeHorCursor;
        
    default:
        return Qt::ArrowCursor;
    }
}

void ImageResizer::resize(const QPointF &pos)
{
    if (!m_target) {
        return;
    }
    
    QPointF delta = pos - m_startPos;
    QRectF rect = m_target->boundingRect();
    QPointF center = m_target->mapToScene(rect.center());
    
    // 应用适合拖动手柄的缩放
    qreal scaleX = 1.0;
    qreal scaleY = 1.0;
    
    // 是否需要调整水平缩放
    bool adjustHorizontal = false;
    
    // 是否需要调整垂直缩放
    bool adjustVertical = false;
    
    // 水平缩放方向
    int hDirection = 0;
    
    // 垂直缩放方向
    int vDirection = 0;
    
    // 确定哪些方向需要调整，以及调整的方向
    switch (m_currentHandle) {
    case TopLeft:
        adjustHorizontal = true;
        adjustVertical = true;
        hDirection = -1; // 左边拖动时，向左扩展
        vDirection = -1; // 顶部拖动时，向上扩展
        break;
        
    case Top:
        adjustVertical = true;
        vDirection = -1; // 向上扩展
        break;
        
    case TopRight:
        adjustHorizontal = true;
        adjustVertical = true;
        hDirection = 1; // 右边拖动时，向右扩展
        vDirection = -1; // 顶部拖动时，向上扩展
        break;
        
    case Right:
        adjustHorizontal = true;
        hDirection = 1; // 向右扩展
        break;
        
    case BottomRight:
        adjustHorizontal = true;
        adjustVertical = true;
        hDirection = 1; // 向右扩展
        vDirection = 1; // 向下扩展
        break;
        
    case Bottom:
        adjustVertical = true;
        vDirection = 1; // 向下扩展
        break;
        
    case BottomLeft:
        adjustHorizontal = true;
        adjustVertical = true;
        hDirection = -1; // 向左扩展
        vDirection = 1; // 向下扩展
        break;
        
    case Left:
        adjustHorizontal = true;
        hDirection = -1; // 向左扩展
        break;
    }
    
    // 计算水平缩放
    if (adjustHorizontal) {
        if (hDirection > 0) {
            // 向右拖动，增加宽度
            scaleX = (m_originalSize.width() + delta.x()) / m_originalSize.width();
        } else {
            // 向左拖动，减少宽度
            scaleX = (m_originalSize.width() - delta.x()) / m_originalSize.width();
        }
    }
    
    // 计算垂直缩放
    if (adjustVertical) {
        if (vDirection > 0) {
            // 向下拖动，增加高度
            scaleY = (m_originalSize.height() + delta.y()) / m_originalSize.height();
        } else {
            // 向上拖动，减少高度
            scaleY = (m_originalSize.height() - delta.y()) / m_originalSize.height();
        }
    }
    
    // 如果用户按下Shift键，保持纵横比
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        if (adjustHorizontal && adjustVertical) {
            qreal maxScale = qMax(scaleX, scaleY);
            scaleX = scaleY = maxScale;
        } else if (adjustHorizontal) {
            scaleY = scaleX;
        } else if (adjustVertical) {
            scaleX = scaleY;
        }
    }
    
    // 设置缩放，确保不至于缩到太小
    const qreal MIN_SCALE = 0.1;
    scaleX = qMax(MIN_SCALE, scaleX);
    scaleY = qMax(MIN_SCALE, scaleY);
    
    // 应用变换
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.scale(scaleX, scaleY);
    transform.translate(-center.x(), -center.y());
    
    // 保存当前旋转角度
    qreal currentRotation = m_target->rotation();
    
    // 设置变换
    m_target->setScale(1.0); // 先重置可能已经应用的缩放
    m_target->setTransform(transform);
    m_target->setRotation(currentRotation); // 恢复旋转
    
    // 更新控制点位置
    updateHandles();
}

void ImageResizer::rotate(const QPointF &pos)
{
    if (!m_target) {
        return;
    }
    
    // 计算当前角度
    QLineF line(m_originalCenter, pos);
    qreal currentAngle = line.angle();
    
    // 计算旋转角度差
    qreal angleDelta = m_startAngle - currentAngle;
    
    // 应用到原始旋转角度
    qreal newRotation = m_originalRotation - angleDelta;
    
    // 如果用户按下了Shift键，则以15度为单位旋转
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        newRotation = qRound(newRotation / 15.0) * 15.0;
    }
    
    // 设置旋转
    m_target->setRotation(newRotation);
    
    // 更新控制点位置
    updateHandles();
} 