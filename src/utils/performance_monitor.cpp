#include "performance_monitor.h"
#include "logger.h"
#include <QApplication>
#include <QFontMetrics>
#include <QDateTime>
#include <QMutexLocker>
#include <algorithm>
#include <QThread>
#include <QTimer>

// PerformanceWorker 实现 ===================================

PerformanceWorker::PerformanceWorker(QObject* parent)
    : QObject(parent)
{
    // 初始化帧率计时器
    m_fpsTimer.start();
    
    Logger::debug("性能监控工作线程已创建");
}

PerformanceWorker::~PerformanceWorker()
{
    Logger::debug("性能监控工作线程已销毁");
}

void PerformanceWorker::processEvent(const PerformanceEvent& event)
{
    // 根据事件类型处理
    switch (event.type) {
        case EVENT_START_MEASURE:
            startMeasure(event.metricType, event.customName);
            break;
            
        case EVENT_END_MEASURE:
            endMeasure(event.metricType);
            break;
            
        case EVENT_FRAME_COMPLETED:
            frameCompleted();
            break;
            
        case EVENT_RESET_DATA:
            resetMeasurements();
            break;
            
        case EVENT_OVERLAY_TOGGLE:
            if (m_overlayEnabled != event.boolValue) {
                m_overlayEnabled = event.boolValue;
                emit overlayToggled(m_overlayEnabled);
            }
            break;
            
        case EVENT_CHANGE_ENABLED:
            if (m_enabled != event.boolValue) {
                m_enabled = event.boolValue;
                
                // 如果刚启用，重置测量
                if (m_enabled) {
                    resetMeasurements();
                }
                
                emit statusChanged(m_enabled);
            }
            break;
    }
}

void PerformanceWorker::processEvents()
{
    // 这个方法会被定时器触发，处理事件队列
    // 实际处理在PerformanceMonitor中进行
}

void PerformanceWorker::stop()
{
    // 执行必要的清理
    m_measurements.clear();
    m_enabled = false;
    m_overlayEnabled = false;
    m_frameCount = 0;
    
    Logger::debug("性能监控工作线程已停止");
}

void PerformanceWorker::startMeasure(int type, const QString& customName)
{
    // 增加启用状态检查并在日志中说明
    if (!m_enabled) {
        Logger::debug("性能监控：startMeasure被调用但监控未启用");
        return;
    }
    
    // 保存自定义名称（如果有）
    if (!customName.isEmpty() && (type == PerformanceMonitor::CustomMetric1 || type == PerformanceMonitor::CustomMetric2)) {
        m_customNames[type] = customName;
    }
    
    // 开始计时
    if (!m_timers.contains(type)) {
        m_timers[type] = QElapsedTimer();
    }
    
    m_timers[type].start();
}

void PerformanceWorker::endMeasure(int type)
{
    // 增加启用状态检查并在日志中说明
    if (!m_enabled) {
        Logger::debug("性能监控：endMeasure被调用但监控未启用");
        return;
    }
    
    if (m_timers.contains(type) && m_timers[type].isValid()) {
        qint64 elapsed = m_timers[type].elapsed();
        recordMeasurement(type, elapsed);
    } else {
        Logger::warning(QString("性能监控：尝试结束未开始的测量，类型=%1").arg(type));
    }
}

void PerformanceWorker::frameCompleted()
{
    if (!m_enabled) return;
    
    // 更新帧计数器
    m_frameCount++;
    
    // 避免帧数异常
    if (m_frameCount > 10000) {
        // 防止计数器溢出或异常值
        m_frameCount = 1;
        m_fpsTimer.restart();
        return;
    }
    
    // 防止计时器异常
    if (!m_fpsTimer.isValid()) {
        m_fpsTimer.start();
        return;
    }
    
    // 获取经过的时间
    qint64 elapsed = m_fpsTimer.elapsed();
    
    // 检查时间是否合理
    if (elapsed <= 0) {
        m_fpsTimer.restart();
        return;
    }
    
    // 每秒更新一次FPS
    if (elapsed >= 1000) {
        m_currentFPS = m_frameCount * 1000 / elapsed;
        
        // 限制FPS范围，避免异常值
        if (m_currentFPS < 0) m_currentFPS = 0;
        if (m_currentFPS > 1000) m_currentFPS = 1000;
        
        m_frameCount = 0;
        m_fpsTimer.restart();
        
        // 记录到测量数据中
        if (m_currentFPS > 0) {
            recordMeasurement(PerformanceMonitor::FrameTime, 1000 / m_currentFPS);
        } else {
            recordMeasurement(PerformanceMonitor::FrameTime, 0);
        }
        
        // 通知数据已更新
        emit measurementsUpdated();
    }
}

void PerformanceWorker::resetMeasurements()
{
    // 清空所有测量数据
    m_measurements.clear();
    m_frameCount = 0;
    
    // 重启帧率计时器
    if (m_fpsTimer.isValid()) {
        m_fpsTimer.restart();
    } else {
        m_fpsTimer.start();
    }
    
    Logger::debug("性能监控测量数据已重置");
    
    // 通知数据已更新
    emit measurementsUpdated();
}

void PerformanceWorker::recordMeasurement(int type, qint64 elapsedMs)
{
    // 检查类型是否有效
    if (type < 0 || type >= PerformanceMonitor::MetricTypeCount) {
        Logger::warning(QString("性能监控：无效的指标类型 %1").arg(type));
        return;
    }
    
    // 检查数值是否合理
    if (elapsedMs < 0 || elapsedMs > 10000) { // 限制最大值为10秒
        Logger::warning(QString("性能监测值异常: %1 ms").arg(elapsedMs));
        elapsedMs = std::min<qint64>(std::max<qint64>(0, elapsedMs), 10000);
    }
    
    // 检查是否已经存在该类型的测量容器，如果没有则创建
    if (!m_measurements.contains(type)) {
        m_measurements[type] = QVector<qint64>();
        // 预分配空间，提高性能
        m_measurements[type].reserve(m_maxSamples);
        Logger::debug(QString("性能监控：为指标类型 %1 创建新的测量容器").arg(type));
    }
    
    // 添加测量值
    m_measurements[type].append(elapsedMs);
    
    // 限制样本数量
    auto& samples = m_measurements[type];
    if (samples.size() > m_maxSamples) {
        samples.removeFirst();
    }
    
    // 每收集20个样本记录一次日志，避免日志过于频繁
    if (samples.size() % 20 == 0) {
        Logger::debug(QString("性能监控：指标类型 %1 已收集 %2 个样本").arg(type).arg(samples.size()));
    }
    
    // 每收集5个样本就发送一次更新信号，确保数据能及时被处理
    if (samples.size() % 5 == 0) {
        emit measurementsUpdated();
    }
}

double PerformanceWorker::calculateAverage(const QVector<qint64>& measurements) const
{
    if (measurements.isEmpty()) return 0.0;
    
    qint64 sum = 0;
    for (qint64 value : measurements) {
        sum += value;
    }
    
    return static_cast<double>(sum) / measurements.size();
}

qint64 PerformanceWorker::calculateMax(const QVector<qint64>& measurements) const
{
    if (measurements.isEmpty()) return 0;
    
    qint64 max = measurements[0];
    for (qint64 value : measurements) {
        if (value > max) max = value;
    }
    
    return max;
}

// PerformanceMonitor 实现 ===================================

// 单例实例获取
PerformanceMonitor& PerformanceMonitor::instance()
{
    static PerformanceMonitor instance;
    return instance;
}

// 构造函数
PerformanceMonitor::PerformanceMonitor(QObject* parent)
    : QObject(parent)
    , m_enabled(false)
    , m_overlayEnabled(false)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_processTimer(nullptr)
{
    m_overlayFont = QFont("Monospace", 8);
    m_overlayFont.setStyleHint(QFont::TypeWriter);
    
    // 默认启用帧率和绘制时间监控
    m_visibleMetrics << FrameTime << DrawTime;
    
    // 创建工作线程和工作对象
    m_workerThread = new QThread(this);
    m_worker = new PerformanceWorker();
    m_worker->moveToThread(m_workerThread);
    
    // 连接信号和槽
    connect(m_workerThread, &QThread::finished, m_worker, &PerformanceWorker::stop);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &PerformanceMonitor::sendEvent, m_worker, &PerformanceWorker::processEvent, Qt::QueuedConnection);
    
    // 连接工作对象的信号
    connect(m_worker, &PerformanceWorker::measurementsUpdated, this, &PerformanceMonitor::dataUpdated);
    connect(m_worker, &PerformanceWorker::statusChanged, this, &PerformanceMonitor::enabledChanged);
    connect(m_worker, &PerformanceWorker::overlayToggled, this, &PerformanceMonitor::overlayEnabledChanged);
    
    // 创建和连接处理定时器
    m_processTimer = new QTimer(this);
    m_processTimer->setInterval(m_processingInterval);
    connect(m_processTimer, &QTimer::timeout, this, &PerformanceMonitor::checkQueue);
    
    // 启动工作线程
    m_workerThread->start();
    
    // 启动事件处理定时器
    m_processTimer->start();
    
    Logger::info("性能监控系统已初始化");
}

// 析构函数
PerformanceMonitor::~PerformanceMonitor()
{
    if (m_processTimer) {
        m_processTimer->stop();
    }
    
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

// 启用/禁用性能监控
void PerformanceMonitor::setEnabled(bool enabled)
{
    if (m_enabled.load() != enabled) {
        // 首先更新本地状态
        m_enabled.store(enabled);
        
        // 然后通过事件队列通知工作线程
        PerformanceEvent event(EVENT_CHANGE_ENABLED);
        event.boolValue = enabled;
        enqueueEvent(event);
        
        Logger::info(QString("性能监控状态切换为%1").arg(enabled ? "启用" : "禁用"));
    }
}

bool PerformanceMonitor::isEnabled() const
{
    return m_enabled.load();
}

// 启用/禁用覆盖层
void PerformanceMonitor::setOverlayEnabled(bool enable)
{
    if (m_overlayEnabled.load() != enable) {
        // 首先更新本地状态
        m_overlayEnabled.store(enable);
        
        // 然后通过事件队列通知工作线程
        PerformanceEvent event(EVENT_OVERLAY_TOGGLE);
        event.boolValue = enable;
        enqueueEvent(event);
        
        Logger::info(QString("性能监控覆盖层已%1").arg(enable ? "启用" : "禁用"));
    }
}

bool PerformanceMonitor::isOverlayEnabled() const
{
    return m_overlayEnabled.load();
}

// 开始测量
void PerformanceMonitor::startMeasure(MetricType type, const QString& customName)
{
    if (!m_enabled.load()) return;
    
    PerformanceEvent event(EVENT_START_MEASURE);
    event.metricType = static_cast<int>(type);
    event.customName = customName;
    enqueueEvent(event);
}

// 结束测量
void PerformanceMonitor::endMeasure(MetricType type)
{
    if (!m_enabled.load()) return;
    
    PerformanceEvent event(EVENT_END_MEASURE);
    event.metricType = static_cast<int>(type);
    enqueueEvent(event);
}

// 帧完成通知
void PerformanceMonitor::frameCompleted()
{
    if (!m_enabled.load()) return;
    
    // 创建并入队帧完成事件
    PerformanceEvent event(EVENT_FRAME_COMPLETED);
    enqueueEvent(event);
}

// 获取当前帧率
int PerformanceMonitor::getFPS() const
{
    if (!m_enabled.load() || !m_worker) return 0;
    return m_worker->m_currentFPS;
}

// 获取平均耗时
double PerformanceMonitor::getAverageTime(MetricType type) const
{
    if (!m_enabled.load() || !m_worker) return 0.0;
    
    if (m_worker->m_measurements.contains(static_cast<int>(type)) && 
        !m_worker->m_measurements[static_cast<int>(type)].isEmpty()) {
        return m_worker->calculateAverage(m_worker->m_measurements[static_cast<int>(type)]);
    }
    return 0.0;
}

// 获取最大耗时
double PerformanceMonitor::getMaxTime(MetricType type) const
{
    if (!m_enabled.load() || !m_worker) return 0.0;
    
    if (m_worker->m_measurements.contains(static_cast<int>(type)) && 
        !m_worker->m_measurements[static_cast<int>(type)].isEmpty()) {
        return m_worker->calculateMax(m_worker->m_measurements[static_cast<int>(type)]);
    }
    return 0.0;
}

// 获取性能报告
QString PerformanceMonitor::getPerformanceReport() const
{
    if (!m_enabled.load() || !m_worker) {
        return "性能监控未启用";
    }
    
    QString report;
    QTextStream stream(&report);
    
    // 使用QDateTime生成时间戳
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString dateTimeStr = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
    
    stream << "========== 性能报告 (" << dateTimeStr << ") ==========\n\n";
    
    // 显示帧率信息
    int currentFPS = m_worker->m_currentFPS;
    double frameTimeAvg = 0.0;
    qint64 frameTimeMax = 0;
    int frameTimeSamples = 0;
    
    if (m_worker->m_measurements.contains(FrameTime) && !m_worker->m_measurements[FrameTime].isEmpty()) {
        const auto& frameTimes = m_worker->m_measurements[FrameTime];
        frameTimeAvg = m_worker->calculateAverage(frameTimes);
        frameTimeMax = m_worker->calculateMax(frameTimes);
        frameTimeSamples = frameTimes.size();
    }
    
    stream << "【帧率信息】\n";
    stream << "当前帧率:    " << currentFPS << " FPS\n";
    if (frameTimeSamples > 0) {
        stream << "平均帧时间:   " << QString::number(frameTimeAvg, 'f', 2) << " ms\n";
        stream << "最大帧时间:   " << frameTimeMax << " ms\n";
        stream << "采样数量:     " << frameTimeSamples << "\n";
    }
    stream << "\n";
    
    // 性能指标类别
    QMap<int, QString> categories;
    categories[UpdateTime] = "【更新性能】";
    categories[EventTime] = "【事件处理】";
    categories[DrawTime] = "【绘制性能】";
    categories[LogicTime] = "【逻辑性能】";
    
    // 遍历所有已测量的指标
    bool hasMetrics = false;
    for (int typeKey : {UpdateTime, EventTime, DrawTime, LogicTime}) {
        if (m_worker->m_measurements.contains(typeKey) && !m_worker->m_measurements[typeKey].isEmpty()) {
            const auto& measurements = m_worker->m_measurements[typeKey];
            
            double avg = m_worker->calculateAverage(measurements);
            qint64 max = m_worker->calculateMax(measurements);
            qint64 latest = measurements.isEmpty() ? 0 : measurements.last();
            int samplesCount = measurements.size();
            
            // 计算标准差
            double variance = 0.0;
            for (qint64 val : measurements) {
                double diff = val - avg;
                variance += diff * diff;
            }
            double stdDev = samplesCount > 1 ? qSqrt(variance / samplesCount) : 0.0;
            
            // 计算稳定性指标 (作为标准差与平均值的比率，越低越稳定)
            double stability = avg > 0 ? 100.0 * (1.0 - qMin(1.0, stdDev / avg)) : 0.0;
            
            stream << categories[typeKey] << "\n";
            stream << "当前耗时:     " << latest << " ms\n";
            stream << "平均耗时:     " << QString::number(avg, 'f', 2) << " ms\n";
            stream << "最大耗时:     " << max << " ms\n";
            stream << "稳定性指标:   " << QString::number(stability, 'f', 1) << "%\n";
            stream << "采样数量:     " << samplesCount << "\n";
            stream << "\n";
            
            hasMetrics = true;
        }
    }
    
    // 显示自定义指标
    for (int customType : {CustomMetric1, CustomMetric2}) {
        if (m_worker->m_measurements.contains(customType) && !m_worker->m_measurements[customType].isEmpty()) {
            QString name = getMetricName(static_cast<MetricType>(customType));
            const auto& measurements = m_worker->m_measurements[customType];
            
            double avg = m_worker->calculateAverage(measurements);
            qint64 max = m_worker->calculateMax(measurements);
            qint64 latest = measurements.isEmpty() ? 0 : measurements.last();
            int samplesCount = measurements.size();
            
            stream << "【" << name << "】\n";
            stream << "当前值:       " << latest << " ms\n";
            stream << "平均值:       " << QString::number(avg, 'f', 2) << " ms\n";
            stream << "最大值:       " << max << " ms\n";
            stream << "采样数量:     " << samplesCount << "\n";
            stream << "\n";
            
            hasMetrics = true;
        }
    }
    
    if (!hasMetrics) {
        stream << "暂无性能指标数据。使用应用程序一段时间后再查看报告。\n";
    }
    
    stream << "=======================================\n";
    stream << "性能监控最大样本数: " << m_worker->m_maxSamples << "\n";
    
    return report;
}

// 设置可见指标
void PerformanceMonitor::setVisibleMetrics(const QVector<MetricType>& metrics)
{
    m_visibleMetrics = metrics;
}

// 重置测量数据
void PerformanceMonitor::resetMeasurements()
{
    if (!m_enabled.load()) return;
    
    PerformanceEvent event(EVENT_RESET_DATA);
    enqueueEvent(event);
}

// 设置采样数
void PerformanceMonitor::setSamplesCount(int samplesCount)
{
    if (!m_worker || samplesCount <= 0) return;
    
    // 添加日志
    Logger::info(QString("性能监控：设置采样数为 %1").arg(samplesCount));
    
    m_worker->m_maxSamples = samplesCount;
    
    // 裁剪现有样本
    for (auto it = m_worker->m_measurements.begin(); it != m_worker->m_measurements.end(); ++it) {
        QVector<qint64>& samples = it.value();
        if (samples.size() > samplesCount) {
            samples = samples.mid(samples.size() - samplesCount);
        }
    }
}

// ScopedTimer实现
PerformanceMonitor::ScopedTimer::ScopedTimer(MetricType type, const QString& customName)
    : m_type(type)
{
    if (PerformanceMonitor::instance().isEnabled()) {
        PerformanceMonitor::instance().startMeasure(type, customName);
    }
}

PerformanceMonitor::ScopedTimer::~ScopedTimer()
{
    if (PerformanceMonitor::instance().isEnabled()) {
        PerformanceMonitor::instance().endMeasure(m_type);
    }
}

// 渲染性能覆盖层
void PerformanceMonitor::renderOverlay(QPainter* painter, const QRectF& rect)
{
    // 基本检查，避免不必要的处理
    if (!m_enabled.load() || !m_overlayEnabled.load() || !painter || !m_worker) return;
    
    try {
        // 保存绘制器状态
        painter->save();
        
        // 设置半透明背景
        QColor bgColor(30, 30, 30, 200);
        painter->fillRect(rect, bgColor);
        painter->setPen(QPen(QColor(100, 100, 100, 200), 1));
        painter->drawRect(rect);
        
        // 在渲染之前复制测量数据（避免长时间锁定）
        QMap<MetricType, QVector<qint64>> renderData;
        
        // 获取工作线程数据的副本
        for (auto it = m_worker->m_measurements.constBegin(); it != m_worker->m_measurements.constEnd(); ++it) {
            if (!it.value().isEmpty() && m_visibleMetrics.contains(static_cast<MetricType>(it.key()))) {
                renderData[static_cast<MetricType>(it.key())] = it.value();
            }
        }
        
        // 使用获取的数据渲染
        if (!renderData.isEmpty()) {
            renderPerformanceGraph(painter, rect, renderData);
        }
        
        // 渲染文本信息
        renderTextInfo(painter, rect);
        
        // 恢复绘制器状态
        painter->restore();
    }
    catch (...) {
        Logger::error("性能覆盖层绘制时发生异常");
        if (painter) {
            painter->restore();
        }
    }
}

// 渲染性能图表
void PerformanceMonitor::renderPerformanceGraph(QPainter* painter, const QRectF& rect, const QMap<MetricType, QVector<qint64>>& data)
{
    // 计算图表区域
    QRectF graphRect(rect.left() + 10, rect.top() + 10, 
                     rect.width() - 20, m_graphHeight);
    
    // 绘制图表背景
    painter->fillRect(graphRect, QColor(20, 20, 20, 220));
    painter->setPen(QPen(QColor(100, 100, 100), 1));
    painter->drawRect(graphRect);
    
    // 找出所有指标的最大值
    qint64 maxVal = 10; // 默认最小值为10ms，确保图表有高度
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        for (qint64 val : it.value()) {
            if (val > maxVal) maxVal = val;
        }
    }
    
    // 向上取整到合适的刻度，使图表更直观
    // 比如把17.3变成20，把83变成100
    int magnitudeOrder = 0;
    while (maxVal >= 100) {
        maxVal /= 10;
        magnitudeOrder++;
    }
    
    // 向上取整到5ms的倍数
    maxVal = ((maxVal + 4) / 5) * 5;
    
    // 还原实际的最大值刻度
    for (int i = 0; i < magnitudeOrder; i++) {
        maxVal *= 10;
    }
    
    // 绘制水平刻度线和标签
    painter->setFont(QFont("Arial", 7));
    QFontMetrics fm(painter->font());
    
    int steps = 5;  // 分成5个刻度
    for (int i = 0; i <= steps; i++) {
        qreal y = graphRect.top() + (graphRect.height() * (steps - i) / steps);
        qint64 value = maxVal * i / steps;
        
        // 绘制刻度线
        painter->setPen(QPen(QColor(70, 70, 70), 1, Qt::DotLine));
        painter->drawLine(QPointF(graphRect.left(), y), QPointF(graphRect.right(), y));
        
        // 绘制刻度标签
        painter->setPen(QColor(200, 200, 200));
        QString label = QString::number(value) + "ms";
        QRectF textRect(graphRect.left() - fm.horizontalAdvance(label) - 5, y - fm.height()/2, 
                        fm.horizontalAdvance(label), fm.height());
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }
    
    // 绘制图例
    QRectF legendRect(graphRect.right() - 100, graphRect.top() + 5, 95, data.size() * 15 + 5);
    painter->fillRect(legendRect, QColor(0, 0, 0, 150));
    painter->setPen(QColor(100, 100, 100));
    painter->drawRect(legendRect);
    
    int legendY = legendRect.top() + 12;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        MetricType type = it.key();
        QColor color = getMetricColor(type);
        
        // 绘制图例颜色方块
        painter->fillRect(QRectF(legendRect.left() + 5, legendY - 5, 10, 10), color);
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(legendRect.left() + 20, legendY, getMetricName(type));
        legendY += 15;
    }
    
    // 对于每个可见指标，绘制图表线
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        MetricType type = it.key();
        QVector<qint64> measurements = it.value();
        
        if (measurements.size() > 1) {
            QColor lineColor = getMetricColor(type);
            painter->setPen(QPen(lineColor, 2));
            
            // 计算点间距
            qreal dx = graphRect.width() / (measurements.size() - 1);
            
            // 绘制线
            QPainterPath path;
            bool first = true;
            
            for (int i = 0; i < measurements.size(); i++) {
                // 计算位置
                qreal x = graphRect.left() + i * dx;
                qreal valueRatio = qMin(1.0, measurements[i] / static_cast<double>(maxVal));
                qreal y = graphRect.bottom() - (valueRatio * graphRect.height());
                
                // 添加到路径
                if (first) {
                    path.moveTo(x, y);
                    first = false;
                } else {
                    path.lineTo(x, y);
                }
                
                // 绘制数据点
                if (measurements.size() < 30) {  // 只在点数较少时绘制圆点，避免拥挤
                    painter->setBrush(lineColor);
                    painter->setPen(Qt::NoPen);
                    painter->drawEllipse(QPointF(x, y), 3, 3);
                    painter->setPen(QPen(lineColor, 2));
                }
            }
            
            // 绘制路径
            painter->drawPath(path);
            
            // 填充区域
            QPainterPath fillPath = path;
            fillPath.lineTo(graphRect.right(), graphRect.bottom());
            fillPath.lineTo(graphRect.left(), graphRect.bottom());
            fillPath.closeSubpath();
            
            QColor fillColor = lineColor;
            fillColor.setAlpha(40);
            painter->fillPath(fillPath, fillColor);
        }
    }
    
    // 绘制当前时间线
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("hh:mm:ss");
    painter->setPen(QColor(200, 200, 200));
    painter->drawText(graphRect.right() - fm.horizontalAdvance(timeStr) - 5, 
                     graphRect.bottom() + fm.height() + 2, timeStr);
}

// 渲染文本信息
void PerformanceMonitor::renderTextInfo(QPainter* painter, const QRectF& rect)
{
    if (!m_worker) return;
    
    // 设置文本绘制参数
    painter->setFont(m_overlayFont);
    
    QFontMetrics fm(m_overlayFont);
    int lineHeight = fm.height() + 2;  // 增加行距
    qreal textX = rect.left() + 15;
    qreal textY = rect.top() + m_graphHeight + 25;  // 增加与图表的间距
    
    // 创建信息显示区域背景
    QRectF infoRect(rect.left() + 5, textY - lineHeight, 
                   rect.width() - 10, rect.height() - (textY - rect.top()) - 5);
    painter->fillRect(infoRect, QColor(0, 0, 0, 120));
    painter->setPen(QPen(QColor(100, 100, 100), 1));
    painter->drawRect(infoRect);
    
    // 标题
    textY += 5;
    painter->setPen(QColor(220, 220, 220));
    painter->setFont(QFont(m_overlayFont.family(), m_overlayFont.pointSize() + 1, QFont::Bold));
    painter->drawText(textX, textY, "实时性能数据");
    textY += lineHeight + 5;  // 标题下方多空一点
    
    // 恢复普通字体
    painter->setFont(m_overlayFont);
    
    // 显示帧率相关信息
    QPen normalPen(Qt::white);
    QPen labelPen(QColor(180, 180, 180));
    QPen valuePen(QColor(220, 220, 220));
    QPen warningPen(QColor(255, 150, 0));
    QPen criticalPen(QColor(255, 80, 80));
    
    // 帧率显示，根据帧率设置不同颜色
    int fps = m_worker->m_currentFPS;
    painter->setPen(labelPen);
    painter->drawText(textX, textY, "FPS:");
    
    // 根据帧率设置不同颜色
    if (fps >= 55) {
        painter->setPen(QPen(QColor(100, 255, 100))); // 高帧率：绿色
    } else if (fps >= 30) {
        painter->setPen(QPen(QColor(255, 255, 100))); // 中等帧率：黄色
    } else {
        painter->setPen(criticalPen); // 低帧率：红色
    }
    
    // 用更大字体显示帧率
    painter->setFont(QFont(m_overlayFont.family(), m_overlayFont.pointSize() + 2, QFont::Bold));
    painter->drawText(textX + 40, textY, QString::number(fps));
    painter->setFont(m_overlayFont);
    textY += lineHeight + 5;  // 帧率下方额外空一点
    
    // 显示其他指标
    const int col1Width = 80;  // 第一列宽度
    const int col2Width = 80;  // 第二列宽度
    const int col3Width = 80;  // 第三列宽度
    
    // 表头
    painter->setPen(labelPen);
    painter->drawText(textX, textY, "指标");
    painter->drawText(textX + col1Width, textY, "当前");
    painter->drawText(textX + col1Width + col2Width, textY, "平均");
    painter->drawText(textX + col1Width + col2Width + col3Width, textY, "最大");
    textY += lineHeight;
    
    // 水平分隔线
    painter->setPen(QPen(QColor(100, 100, 100), 1));
    painter->drawLine(QPointF(textX, textY - lineHeight/2), 
                     QPointF(textX + col1Width + col2Width + col3Width + 50, textY - lineHeight/2));
    
    // 排序指标类型，按顺序显示
    QList<MetricType> orderedMetrics = {DrawTime, UpdateTime, EventTime, LogicTime};
    
    // 显示每个指标
    for (MetricType type : orderedMetrics) {
        if (m_worker->m_measurements.contains(static_cast<int>(type)) && 
            !m_worker->m_measurements[static_cast<int>(type)].isEmpty() &&
            m_visibleMetrics.contains(type)) {
            
            const auto& measurements = m_worker->m_measurements[static_cast<int>(type)];
            double avg = m_worker->calculateAverage(measurements);
            qint64 max = m_worker->calculateMax(measurements);
            qint64 current = measurements.isEmpty() ? 0 : measurements.last();
            
            // 指标名称
            painter->setPen(getMetricColor(type));
            QString name = getMetricName(type);
            painter->drawText(textX, textY, name);
            
            // 当前值
            painter->setPen(current > avg*1.5 ? warningPen : valuePen);
            painter->drawText(textX + col1Width, textY, QString::number(current) + " ms");
            
            // 平均值
            painter->setPen(valuePen);
            painter->drawText(textX + col1Width + col2Width, textY, 
                             QString::number(avg, 'f', 1) + " ms");
            
            // 最大值
            painter->setPen(max > avg*2 ? warningPen : valuePen);
            painter->drawText(textX + col1Width + col2Width + col3Width, textY, 
                             QString::number(max) + " ms");
            
            textY += lineHeight;
        }
    }
    
    // 显示自定义指标
    for (int customType : {CustomMetric1, CustomMetric2}) {
        if (m_worker->m_measurements.contains(customType) && 
            !m_worker->m_measurements[customType].isEmpty() &&
            m_visibleMetrics.contains(static_cast<MetricType>(customType))) {
            
            const auto& measurements = m_worker->m_measurements[customType];
            double avg = m_worker->calculateAverage(measurements);
            qint64 max = m_worker->calculateMax(measurements);
            qint64 current = measurements.isEmpty() ? 0 : measurements.last();
            
            // 指标名称
            painter->setPen(getMetricColor(static_cast<MetricType>(customType)));
            QString name = getMetricName(static_cast<MetricType>(customType));
            painter->drawText(textX, textY, name);
            
            // 当前值
            painter->setPen(current > avg*1.5 ? warningPen : valuePen);
            painter->drawText(textX + col1Width, textY, QString::number(current) + " ms");
            
            // 平均值
            painter->setPen(valuePen);
            painter->drawText(textX + col1Width + col2Width, textY, 
                             QString::number(avg, 'f', 1) + " ms");
            
            // 最大值
            painter->setPen(max > avg*2 ? warningPen : valuePen);
            painter->drawText(textX + col1Width + col2Width + col3Width, textY, 
                             QString::number(max) + " ms");
            
            textY += lineHeight;
        }
    }
    
    // 添加监控状态信息
    textY += 5;
    painter->setPen(QColor(150, 150, 150));
    painter->drawText(textX, textY, QString("采样: %1").arg(m_worker->m_maxSamples));
}

// 获取指标名称
QString PerformanceMonitor::getMetricName(MetricType type) const
{
    switch (type) {
        case FrameTime: return "帧时间";
        case UpdateTime: return "更新耗时";
        case EventTime: return "事件处理";
        case DrawTime: return "绘制耗时";
        case LogicTime: return "逻辑耗时";
        case CustomMetric1:
            return m_worker && m_worker->m_customNames.contains(static_cast<int>(type)) 
                   ? m_worker->m_customNames[static_cast<int>(type)] : "自定义1";
        case CustomMetric2:
            return m_worker && m_worker->m_customNames.contains(static_cast<int>(type)) 
                   ? m_worker->m_customNames[static_cast<int>(type)] : "自定义2";
        default: return "未知";
    }
}

// 获取指标颜色
QColor PerformanceMonitor::getMetricColor(MetricType type) const
{
    switch (type) {
        case FrameTime: return QColor(255, 255, 255); // 白色
        case UpdateTime: return QColor(0, 255, 0);    // 绿色
        case EventTime: return QColor(255, 255, 0);   // 黄色
        case DrawTime: return QColor(0, 200, 255);    // 青色
        case LogicTime: return QColor(255, 128, 0);   // 橙色
        case CustomMetric1: return QColor(255, 0, 255); // 紫色
        case CustomMetric2: return QColor(128, 128, 255); // 紫蓝色
        default: return QColor(150, 150, 150);        // 灰色
    }
}

// 将事件添加到队列
void PerformanceMonitor::enqueueEvent(const PerformanceEvent& event)
{
    if (!m_worker) return;
    
    QMutexLocker locker(&m_queueMutex);
    
    // 限制队列长度，防止内存爆炸
    if (m_eventQueue.size() > 1000) {
        m_eventQueue.clear();
        Logger::warning("性能监控事件队列溢出，已清空");
    }
    
    m_eventQueue.enqueue(event);
    m_queueCondition.wakeOne();
}

// 检查并处理事件队列
void PerformanceMonitor::checkQueue()
{
    if (!m_worker) {
        Logger::warning("性能监控：工作线程对象为空");
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    
    // 如果队列为空，直接返回
    if (m_eventQueue.isEmpty()) return;
    
    // 获取所有事件的副本
    QQueue<PerformanceEvent> events = m_eventQueue;
    m_eventQueue.clear();
    
    // 解锁以便在处理事件时不会阻塞
    locker.unlock();
    
    // 发送事件到工作线程处理
    for (const auto& event : events) {
        emit sendEvent(event);
    }
}
