#include "performance_monitor.h"
#include "logger.h"
#include <QApplication>
#include <QFontMetrics>
#include <QDateTime>
#include <QMutexLocker>
#include <algorithm>
#include <QThread>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

// 初始化静态成员
PerformanceMonitor* PerformanceMonitor::s_instancePtr = nullptr;

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
            Logger::debug(QString("开始测量指标类型: %1").arg(event.metricType));
            startMeasure(event.metricType, event.customName);
            break;
            
        case EVENT_END_MEASURE:
            Logger::debug(QString("结束测量指标类型: %1").arg(event.metricType));
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
            
        case EVENT_CUSTOM_METRIC:
            recordCustomMeasurement(event.customName, event.value);
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
    if (!m_enabled) {
        return;
    }
    
    QWriteLocker locker(&m_dataLock);
    
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
    if (!m_enabled) {
        return;
    }
    
    QWriteLocker locker(&m_dataLock);
    
    if (m_timers.contains(type) && m_timers[type].isValid()) {
        qint64 elapsed = m_timers[type].elapsed();
        // 在锁内调用，不需要再加锁
        recordMeasurement(type, elapsed);
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
        int newFPS = m_frameCount * 1000 / elapsed;
        
        // 限制FPS范围，避免异常值
        if (newFPS < 0) newFPS = 0;
        if (newFPS > 1000) newFPS = 1000;
        
        m_currentFPS.store(newFPS);
        m_frameCount = 0;
        m_fpsTimer.restart();
        
        // 使用写锁保护测量数据
        QWriteLocker locker(&m_dataLock);
        
        // 记录到测量数据中
        if (newFPS > 0) {
            recordMeasurement(PerformanceMonitor::FrameTime, 1000 / newFPS);
        } else {
            recordMeasurement(PerformanceMonitor::FrameTime, 0);
        }
        
        // 通知数据已更新
        emit measurementsUpdated();
    }
}

void PerformanceWorker::resetMeasurements()
{
    QWriteLocker locker(&m_dataLock);
    
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
    // 注意：此方法假设调用时已经获得了写锁
    
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
        m_measurements[type] = QList<qint64>();
        // 预分配空间，提高性能
        m_measurements[type].reserve(m_maxSamples);
    }
    
    // 添加测量值
    m_measurements[type].append(elapsedMs);
    
    // 限制样本数量
    auto& samples = m_measurements[type];
    if (samples.size() > m_maxSamples) {
        samples.removeFirst();
    }
    
    // 记录每个测量的日志，便于调试
    Logger::debug(QString("记录性能指标: 类型=%1, 值=%2ms, 样本数=%3")
                    .arg(type)
                    .arg(elapsedMs)
                    .arg(samples.size()));
    
    // 防止日志过多，只在第一次和每10个测量值记录一次平均值
    if (samples.size() == 1 || samples.size() % 10 == 0) {
        double avg = calculateAverage(samples);
        Logger::debug(QString("性能指标类型 %1 平均值: %2 ms, 样本数: %3")
                      .arg(type)
                      .arg(avg, 0, 'f', 2)
                      .arg(samples.size()));
    }
}

double PerformanceWorker::calculateAverage(const QList<qint64>& measurements) const
{
    if (measurements.isEmpty()) return 0.0;
    
    qint64 sum = 0;
    for (qint64 value : measurements) {
        sum += value;
    }
    
    return static_cast<double>(sum) / measurements.size();
}

qint64 PerformanceWorker::calculateMax(const QList<qint64>& measurements) const
{
    if (measurements.isEmpty()) return 0;
    
    qint64 max = measurements[0];
    for (qint64 value : measurements) {
        if (value > max) max = value;
    }
    
    return max;
}

// 新增的安全数据访问方法
QMap<int, QList<qint64>> PerformanceWorker::getMeasurementsCopy() const
{
    QReadLocker locker(&m_dataLock);
    return m_measurements;
}

int PerformanceWorker::getCurrentFPS() const
{
    return m_currentFPS.load();
}

QList<qint64> PerformanceWorker::getMeasurementData(int type) const
{
    QReadLocker locker(&m_dataLock);
    if (m_measurements.contains(type)) {
        return m_measurements[type];
    }
    return QList<qint64>();
}

QString PerformanceWorker::getCustomName(int type) const
{
    QReadLocker locker(&m_dataLock);
    if (m_customNames.contains(type)) {
        return m_customNames[type];
    }
    return QString();
}

// PerformanceWorker的setMaxSamples方法实现
void PerformanceWorker::setMaxSamples(int samplesCount)
{
    if (samplesCount <= 0) return;
    
    QWriteLocker locker(&m_dataLock);
    
    m_maxSamples = samplesCount;
    
    // 裁剪现有样本
    for (auto it = m_measurements.begin(); it != m_measurements.end(); ++it) {
        QList<qint64>& samples = it.value();
        if (samples.size() > samplesCount) {
            samples = samples.mid(samples.size() - samplesCount);
        }
    }
    
    Logger::debug(QString("性能监控：最大样本数已设置为 %1").arg(samplesCount));
}

// PerformanceMonitor 实现 ===================================

// 单例实例获取
PerformanceMonitor& PerformanceMonitor::instance()
{
    // 使用互斥锁保护初始化
    static QMutex instanceMutex;
    static bool firstRun = true;
    
    // 首先快速检查实例是否已创建
    if (!s_instancePtr) {
        // 如果没有创建，使用互斥锁保护创建过程
        QMutexLocker locker(&instanceMutex);
        
        // 双重检查，确保在获取锁期间没有其他线程创建实例
        if (!s_instancePtr) {
            try {
                // 创建新实例
                s_instancePtr = new PerformanceMonitor(nullptr);
                
                // 安全检查，首次运行时输出日志
                if (firstRun) {
                    qDebug() << "PerformanceMonitor: First instantiation";
                    firstRun = false;
                }
            } catch (const std::exception& e) {
                // 实例化失败时抛出异常前输出错误信息
                qCritical() << "PerformanceMonitor instantiation failed:" << e.what();
                throw;
            } catch (...) {
                qCritical() << "PerformanceMonitor instantiation failed with unknown error";
                throw;
            }
        }
    }
    
    // 如果实例仍然为空（可能由于某种意外），抛出异常
    if (!s_instancePtr) {
        throw std::runtime_error("Failed to create PerformanceMonitor instance");
    }
    
    return *s_instancePtr;
}

// 检查实例是否已创建
bool PerformanceMonitor::isInstanceCreated()
{
    return s_instancePtr != nullptr;
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
    // 设置字体
    m_overlayFont = QFont("Monospace", 8);
    m_overlayFont.setStyleHint(QFont::TypeWriter);
    
    // 默认启用帧率和绘制时间监控
    m_visibleMetrics << FrameTime << DrawTime;
    
    try {
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
        
        // 启动工作线程和定时器
        m_workerThread->start();
        m_processTimer->start();
        
        // 初始化系统资源监控
        initializeResourceMonitoring();
        
        Logger::info("性能监控系统已初始化");
    } catch (const std::exception& e) {
        Logger::error(QString("性能监控初始化失败: %1").arg(e.what()));
    } catch (...) {
        Logger::error("性能监控初始化发生未知异常");
    }
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
    
    PerformanceEvent event(EVENT_FRAME_COMPLETED);
    enqueueEvent(event);
}

// 记录自定义事件
void PerformanceMonitor::recordEvent(const QString& name, qint64 value)
{
    if (!m_enabled.load() || name.isEmpty()) return;
    
    PerformanceEvent event(EVENT_CUSTOM_METRIC);
    event.customName = name;
    event.value = value;
    enqueueEvent(event);
}

// 获取当前帧率
int PerformanceMonitor::getFPS() const
{
    if (!m_enabled.load() || !m_worker) return 0;
    return m_worker->getCurrentFPS();
}

// 获取指标的平均耗时
double PerformanceMonitor::getAverageTime(MetricType type) const
{
    if (!m_enabled.load() || !m_worker) return 0.0;
    
    QList<qint64> measurements = m_worker->getMeasurementData(static_cast<int>(type));
    if (measurements.isEmpty()) return 0.0;
    
    double sum = 0.0;
    for (qint64 value : measurements) {
        sum += value;
    }
    
    return sum / measurements.size();
}

// 获取指标的最大耗时
double PerformanceMonitor::getMaxTime(MetricType type) const
{
    if (!m_enabled.load() || !m_worker) return 0.0;
    
    QList<qint64> measurements = m_worker->getMeasurementData(static_cast<int>(type));
    if (measurements.isEmpty()) return 0.0;
    
    qint64 max = measurements[0];
    for (qint64 value : measurements) {
        if (value > max) max = value;
    }
    
    return static_cast<double>(max);
}

// 生成性能报告文本
QString PerformanceMonitor::getPerformanceReport() const
{
    if (!m_enabled.load() || !m_worker) {
        return "性能监控未启用";
    }
    
    // 确保所有事件已处理并获取最新数据
    const_cast<PerformanceMonitor*>(this)->flushEvents();
    ensureDataSynced();
    
    QString report;
    QTextStream stream(&report);
    QDateTime currentDateTime = QDateTime::currentDateTime();
    
    stream << "========== 性能报告 (" << currentDateTime.toString("yyyy-MM-dd hh:mm:ss") << ") ==========\n\n";
    
    // 帧率信息
    int currentFPS = m_worker->getCurrentFPS();
    QList<qint64> frameTimes = m_worker->getMeasurementData(FrameTime);
    double frameTimeAvg = 0.0;
    qint64 frameTimeMax = 0;
    
    if (!frameTimes.isEmpty()) {
        double sum = 0;
        frameTimeMax = frameTimes[0];
        
        for (qint64 value : frameTimes) {
            sum += value;
            if (value > frameTimeMax) frameTimeMax = value;
        }
        
        frameTimeAvg = sum / frameTimes.size();
    }
    
    stream << "【帧率信息】\n";
    stream << "当前帧率:    " << currentFPS << " FPS\n";
    if (!frameTimes.isEmpty()) {
        stream << "平均帧时间:   " << QString::number(frameTimeAvg, 'f', 2) << " ms\n";
        stream << "最大帧时间:   " << frameTimeMax << " ms\n";
        stream << "采样数量:     " << frameTimes.size() << "\n";
    }
    stream << "\n";
    
    // 性能指标分类
    QMap<int, QString> categories;
    categories[UpdateTime] = "【更新性能】";
    categories[EventTime] = "【事件处理】";
    categories[DrawTime] = "【绘制性能】";
    categories[LogicTime] = "【逻辑性能】";
    categories[ShapesDrawTime] = "【图形绘制】";
    categories[PathProcessTime] = "【路径处理】";
    categories[RenderPrepTime] = "【渲染准备】";
    categories[CpuUsage] = "【CPU使用率】";
    categories[MemoryUsage] = "【内存使用】";
    categories[ThreadCount] = "【线程数量】";
    
    // 输出各类指标数据
    bool hasMetrics = false;
    for (auto it = categories.constBegin(); it != categories.constEnd(); ++it) {
        QList<qint64> measurements = m_worker->getMeasurementData(it.key());
        
        if (!measurements.isEmpty()) {
            // 计算统计数据
            double sum = 0;
            qint64 max = measurements[0];
            qint64 latest = measurements.last();
            
            for (qint64 value : measurements) {
                sum += value;
                if (value > max) max = value;
            }
            
            double avg = sum / measurements.size();
            
            // 计算标准差
            double variance = 0.0;
            for (qint64 val : measurements) {
                double diff = val - avg;
                variance += diff * diff;
            }
            double stdDev = measurements.size() > 1 ? qSqrt(variance / measurements.size()) : 0.0;
            
            // 计算稳定性指标
            double stability = avg > 0 ? 100.0 * (1.0 - qMin(1.0, stdDev / avg)) : 0.0;
            
            stream << it.value() << "\n";
            stream << "当前值:       " << latest << " ms\n";
            stream << "平均值:       " << QString::number(avg, 'f', 2) << " ms\n";
            stream << "最大值:       " << max << " ms\n";
            stream << "稳定性指标:   " << QString::number(stability, 'f', 1) << "%\n";
            stream << "采样数量:     " << measurements.size() << "\n";
            stream << "\n";
            
            hasMetrics = true;
        }
    }
    
    // 自定义指标
    for (int customType : {CustomMetric1, CustomMetric2}) {
        QList<qint64> measurements = m_worker->getMeasurementData(static_cast<int>(customType));
        
        if (!measurements.isEmpty()) {
            QString name = getMetricName(static_cast<MetricType>(customType));
            
            double sum = 0;
            qint64 max = measurements[0];
            qint64 latest = measurements.last();
            
            for (qint64 value : measurements) {
                sum += value;
                if (value > max) max = value;
            }
            
            double avg = sum / measurements.size();
            
            stream << "【" << name << "】\n";
            stream << "当前值:       " << latest << " ms\n";
            stream << "平均值:       " << QString::number(avg, 'f', 2) << " ms\n";
            stream << "最大值:       " << max << " ms\n";
            stream << "采样数量:     " << measurements.size() << "\n";
            stream << "\n";
            
            hasMetrics = true;
        }
    }
    
    // 自定义事件数据
    QMap<QString, QList<qint64>> customEvents = m_worker->getCustomEventData();
    for (auto it = customEvents.constBegin(); it != customEvents.constEnd(); ++it) {
        const QString& name = it.key();
        const QList<qint64>& data = it.value();
        
        if (!data.isEmpty()) {
            double sum = 0;
            qint64 max = data[0];
            qint64 latest = data.last();
            
            for (qint64 value : data) {
                sum += value;
                if (value > max) max = value;
            }
            
            double avg = sum / data.size();
            
            stream << "【" << name << "】\n";
            stream << "当前值:       " << latest << "\n";
            stream << "平均值:       " << QString::number(avg, 'f', 2) << "\n";
            stream << "最大值:       " << max << "\n";
            stream << "采样数量:     " << data.size() << "\n";
            stream << "\n";
            
            hasMetrics = true;
        }
    }
    
    if (!hasMetrics) {
        stream << "暂无性能指标数据。使用应用程序一段时间后再查看报告。\n";
    }
    
    stream << "=======================================\n";
    stream << "性能监控最大样本数: " << m_worker->getMaxSamples() << "\n";
    
    return report;
}

// 设置可见指标
void PerformanceMonitor::setVisibleMetrics(const QVector<MetricType>& metrics)
{
    m_visibleMetrics.clear();
    for (const MetricType& metric : metrics) {
        m_visibleMetrics.insert(metric);
    }
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
    
    // 使用线程安全方法设置样本数
    m_worker->setMaxSamples(samplesCount);
    
    Logger::info(QString("性能监控：设置采样数为 %1").arg(samplesCount));
}

// 渲染性能覆盖层
void PerformanceMonitor::renderOverlay(QPainter* painter, const QRectF& rect)
{
    // 基本检查
    if (!m_enabled.load() || !m_overlayEnabled.load() || !painter || !m_worker) return;
    
    try {
        // 计算覆盖层位置和保存状态
        QRectF overlayRect = calculateOverlayRect(rect);
        painter->save();
        
        // 启用抗锯齿
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        
        // 设置背景
        QColor bgColor(30, 30, 30, qRound(m_overlayOpacity * 255));
        QPainterPath bgPath;
        bgPath.addRoundedRect(overlayRect, 8, 8);
        painter->fillPath(bgPath, bgColor);
        painter->setPen(QPen(QColor(100, 100, 100, qRound(m_overlayOpacity * 255)), 1.5));
        painter->drawPath(bgPath);
        
        // 绘制标题
        QFont titleFont = m_overlayFont;
        titleFont.setPointSize(m_overlayFont.pointSize() + 2);
        titleFont.setBold(true);
        painter->setFont(titleFont);
        painter->setPen(QColor(220, 220, 220));
        
        QFontMetrics titleFm(titleFont);
        int titleHeight = titleFm.height();
        
        QString titleText = "性能监控";
        switch (m_chartMode) {
            case LineChart: titleText += " [折线图]"; break;
            case BarChart: titleText += " [柱状图]"; break;
            case AreaChart: titleText += " [面积图]"; break;
            case DotChart: titleText += " [散点图]"; break;
        }
        
        painter->drawText(overlayRect.left() + 12, overlayRect.top() + titleHeight + 2, titleText);
        
        // 绘制时间戳
        painter->setFont(m_overlayFont);
        QDateTime now = QDateTime::currentDateTime();
        QString timeStr = now.toString("yyyy-MM-dd hh:mm:ss");
        QFontMetrics fm(m_overlayFont);
        painter->setPen(QColor(180, 180, 180));
        painter->drawText(overlayRect.right() - fm.horizontalAdvance(timeStr) - 12, 
                         overlayRect.top() + titleHeight + 2, timeStr);
        
        // 绘制内容
        QRectF contentRect = overlayRect.adjusted(10, titleHeight + 10, -10, -10);
        ensureDataSynced();
        
        if (!m_renderData.isEmpty()) {
            QRectF graphRect = contentRect;
            graphRect.setHeight(m_graphHeight);
            renderPerformanceGraph(painter, graphRect, m_renderData);
            
            // 渲染文本信息
            QRectF textRect = contentRect;
            textRect.setTop(graphRect.bottom() + 10);
            renderTextInfo(painter, textRect);
        } else {
            // 无数据提示
            painter->setPen(QColor(200, 200, 200));
            painter->drawText(contentRect, Qt::AlignCenter, "正在收集性能数据...");
        }
        
        // 恢复状态
        painter->restore();
    }
    catch (const std::exception& e) {
        Logger::error(QString("性能覆盖层绘制时发生异常: %1").arg(e.what()));
        if (painter) painter->restore();
    }
    catch (...) {
        Logger::error("性能覆盖层绘制时发生未知异常");
        if (painter) painter->restore();
    }
}

// 渲染性能图表
void PerformanceMonitor::renderPerformanceGraph(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data)
{
    // 根据显示模式绘制图表
    renderChartByMode(painter, rect, data);
}

// 根据显示模式绘制图表
void PerformanceMonitor::renderChartByMode(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data)
{
    // 计算所有指标的最大值
    qint64 maxVal = 1; // 默认最小值为1ms，确保图表有高度
    bool hasData = false;
    
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        if (it.value().isEmpty()) continue;
        
        hasData = true;
        for (qint64 val : it.value()) {
            if (val > maxVal) maxVal = val;
        }
    }
    
    // 如果没有数据或所有数据都是0，设置一个合理的默认值
    if (!hasData || maxVal <= 1) {
        maxVal = 16; // 默认16ms (对应60fps)
    } else {
        // 向上取整到合适的刻度，使图表更直观
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
        
        // 确保最小值至少为16ms
        maxVal = qMax<qint64>(maxVal, 16);
    }
    
    // 根据当前设置的模式调用对应的绘制方法
    switch (m_chartMode) {
        case LineChart:
            renderLineChart(painter, rect, data, maxVal);
            break;
            
        case BarChart:
            renderBarChart(painter, rect, data, maxVal);
            break;
            
        case AreaChart:
            renderAreaChart(painter, rect, data, maxVal);
            break;
            
        case DotChart:
            renderDotChart(painter, rect, data, maxVal);
            break;
            
        default:
            renderLineChart(painter, rect, data, maxVal);
            break;
    }
}

// 折线图渲染
void PerformanceMonitor::renderLineChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal)
{
    QRectF graphRect = rect;
    
    // 绘制网格线
    drawChartGrid(painter, graphRect, maxVal);
    
    // 图例
    drawChartLegend(painter, graphRect, data);
    
    // 对于每个指标，绘制折线
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        MetricType type = it.key();
        QList<qint64> measurements = it.value();
        
        if (measurements.isEmpty()) continue;
        
        QColor lineColor = getMetricColor(type);
        
        // 设置线宽和样式
        QPen linePen(lineColor, 2.0);
        linePen.setCapStyle(Qt::RoundCap);
        linePen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(linePen);
        
        // 只有一个点的情况 - 绘制一个数据点
        if (measurements.size() == 1) {
            qreal x = graphRect.center().x();
            qreal valueRatio = qMin(1.0, measurements[0] / static_cast<double>(maxVal));
            qreal y = graphRect.bottom() - (valueRatio * graphRect.height());
            
            painter->setBrush(lineColor);
            painter->drawEllipse(QPointF(x, y), 4, 4);
            
            // 绘制说明文本
            painter->setPen(QColor(220, 220, 220));
            QString valueText = QString::number(measurements[0]) + "ms";
            painter->drawText(QPointF(x + 8, y), valueText);
            
            continue;
        }
        
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
            
            // 仅在点数较少时显示数据点标记
            if (measurements.size() < 30 || i % (measurements.size() / 10 + 1) == 0) {
                painter->setBrush(lineColor);
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(QPointF(x, y), 3, 3);
                painter->setPen(linePen);
                
                // 对于少量数据点，添加数值标签
                if (measurements.size() <= 8 && measurements[i] > 0) {
                    painter->setPen(QColor(220, 220, 220));
                    QString valueText = QString::number(measurements[i]) + "ms";
                    painter->drawText(QPointF(x - 10, y - 8), valueText);
                    painter->setPen(linePen);
                }
            }
        }
        
        // 平滑绘制路径
        painter->setPen(linePen);
        painter->drawPath(path);
    }
    
    // 添加当前时间
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("hh:mm:ss");
    painter->setPen(QColor(200, 200, 200));
    QFontMetrics fm(painter->font());
    painter->drawText(
        graphRect.right() - fm.horizontalAdvance(timeStr) - 5, 
        graphRect.bottom() + fm.height() + 2, 
        timeStr
    );
}

// 渲染文本信息
void PerformanceMonitor::renderTextInfo(QPainter* painter, const QRectF& rect)
{
    QFontMetrics fm(m_overlayFont);
    painter->setFont(m_overlayFont);
    
    int lineHeight = fm.height() + 2;
    int textX = rect.x();
    int textY = rect.y() + lineHeight;
    
    // 计算列宽
    int col1Width = 80; // 指标名称列
    int col2Width = 60; // 当前值列
    int col3Width = 60; // 平均值列
    int col4Width = 60; // 最大值列
    
    // 创建表格背景
    QColor headerBgColor(60, 60, 70, 180);
    QRectF headerRect(textX, textY - lineHeight, 
                    col1Width + col2Width + col3Width + col4Width, lineHeight);
    painter->fillRect(headerRect, headerBgColor);
    
    // 标题行
    QPen headerPen(QColor(220, 220, 220));
    QPen valuePen(QColor(255, 255, 255));
    QPen warningPen(QColor(255, 100, 100));
    QPen goodPen(QColor(100, 255, 100));
    
    painter->setPen(headerPen);
    painter->drawText(textX + 5, textY, "指标");
    painter->drawText(textX + col1Width + 5, textY, "当前");
    painter->drawText(textX + col1Width + col2Width + 5, textY, "平均");
    painter->drawText(textX + col1Width + col2Width + col3Width + 5, textY, "最大");
    
    textY += lineHeight;
    
    // 绘制行分隔线
    painter->setPen(QPen(QColor(80, 80, 90), 1));
    painter->drawLine(QPointF(textX, textY - lineHeight/2), 
                     QPointF(textX + col1Width + col2Width + col3Width + col4Width, 
                            textY - lineHeight/2));
    
    // 帧率显示，根据帧率设置不同颜色
    int fps = m_worker->getCurrentFPS();
    
    // 帧率行背景
    QColor rowBgColor = (textY / lineHeight) % 2 == 0 ? 
                      QColor(40, 40, 50, 180) : QColor(50, 50, 60, 180);
    QRectF rowRect(textX, textY - lineHeight, 
                 col1Width + col2Width + col3Width + col4Width, lineHeight);
    painter->fillRect(rowRect, rowBgColor);
    
    // 绘制FPS信息
    painter->setPen(QColor(220, 220, 220));
    painter->drawText(textX + 5, textY, "FPS:");
    
    // 根据帧率设置颜色
    if (fps >= 50) {
        painter->setPen(goodPen);
    } else if (fps >= 30) {
        painter->setPen(QColor(255, 230, 100)); // 黄色 (中等帧率)
    } else {
        painter->setPen(warningPen);
    }
    
    // 添加FPS值及颜色指示器
    painter->drawText(textX + col1Width + 5, textY, QString::number(fps));
    
    // 对应帧时间也一并显示
    QList<qint64> frameTimes = m_worker->getMeasurementData(static_cast<int>(FrameTime));
    if (!frameTimes.isEmpty()) {
        double avgFrameTime = 0;
        for (qint64 time : frameTimes) {
            avgFrameTime += time;
        }
        avgFrameTime /= frameTimes.size();
        
        painter->setPen(valuePen);
        painter->drawText(textX + col1Width + col2Width + 5, textY, 
                       QString::number(avgFrameTime, 'f', 1) + " ms");
    }
    
    textY += lineHeight;
    
    // 显示各项指标
    int rowIndex = 1;
    for (MetricType type : {UpdateTime, EventTime, DrawTime, LogicTime}) {
        if (m_visibleMetrics.contains(type)) {
            QList<qint64> measurements = m_worker->getMeasurementData(static_cast<int>(type));
            
            if (measurements.isEmpty()) {
                continue;
            }
            
            // 交替行背景
            rowBgColor = (rowIndex % 2 == 0) ? 
                        QColor(40, 40, 50, 180) : QColor(50, 50, 60, 180);
            rowRect = QRectF(textX, textY - lineHeight, 
                          col1Width + col2Width + col3Width + col4Width, lineHeight);
            painter->fillRect(rowRect, rowBgColor);
            rowIndex++;
            
            // 计算统计数据
            double sum = 0;
            qint64 max = measurements[0];
            qint64 current = measurements.last();
            
            for (qint64 value : measurements) {
                sum += value;
                if (value > max) max = value;
            }
            
            double avg = sum / measurements.size();
            
            // 设置指标颜色
            painter->setPen(getMetricColor(type));
            
            // 绘制指标名称
            painter->drawText(textX + 5, textY, getMetricName(type));
            
            // 当前值 - 使用颜色指示性能状态
            QPen currentPen = valuePen;
            if (current > avg*1.5) {
                currentPen = warningPen;  // 红色警告
            } else if (current < avg*0.8) {
                currentPen = goodPen;     // 绿色良好
            }
            
            painter->setPen(currentPen);
            painter->drawText(textX + col1Width + 5, textY, 
                           QString::number(current) + " ms");
            
            // 平均值
            painter->setPen(valuePen);
            painter->drawText(textX + col1Width + col2Width + 5, textY, 
                           QString::number(avg, 'f', 1) + " ms");
            
            // 最大值 - 如果明显高于平均值，显示为红色警告
            painter->setPen(max > avg*2 ? warningPen : valuePen);
            painter->drawText(textX + col1Width + col2Width + col3Width + 5, textY, 
                           QString::number(max) + " ms");
            
            textY += lineHeight;
        }
    }
    
    // 显示自定义指标
    for (MetricType customType : {CustomMetric1, CustomMetric2}) {
        if (m_visibleMetrics.contains(customType)) {
            QList<qint64> measurements = m_worker->getMeasurementData(static_cast<int>(customType));
            
            if (measurements.isEmpty()) {
                continue;
            }
            
            // 交替行背景
            rowBgColor = (rowIndex % 2 == 0) ? 
                        QColor(40, 40, 50, 180) : QColor(50, 50, 60, 180);
            rowRect = QRectF(textX, textY - lineHeight, 
                          col1Width + col2Width + col3Width + col4Width, lineHeight);
            painter->fillRect(rowRect, rowBgColor);
            rowIndex++;
            
            // 计算统计数据
            double sum = 0;
            qint64 max = measurements[0];
            qint64 current = measurements.last();
            
            for (qint64 value : measurements) {
                sum += value;
                if (value > max) max = value;
            }
            
            double avg = sum / measurements.size();
            
            // 指标名称
            painter->setPen(getMetricColor(customType));
            QString name = getMetricName(customType);
            painter->drawText(textX + 5, textY, name);
            
            // 当前值
            painter->setPen(current > avg*1.5 ? warningPen : valuePen);
            painter->drawText(textX + col1Width + 5, textY, 
                           QString::number(current) + " ms");
            
            // 平均值
            painter->setPen(valuePen);
            painter->drawText(textX + col1Width + col2Width + 5, textY, 
                           QString::number(avg, 'f', 1) + " ms");
            
            // 最大值
            painter->setPen(max > avg*2 ? warningPen : valuePen);
            painter->drawText(textX + col1Width + col2Width + col3Width + 5, textY, 
                           QString::number(max) + " ms");
            
            textY += lineHeight;
        }
    }
    
    // 表格底部
    painter->setPen(QPen(QColor(80, 80, 90), 1));
    painter->drawLine(QPointF(textX, textY - lineHeight/2), 
                     QPointF(textX + col1Width + col2Width + col3Width + col4Width, 
                            textY - lineHeight/2));
    
    // 添加状态栏信息
    textY += 5;
    painter->setPen(QColor(180, 180, 180));
    
    // 计算当前收集的样本总数
    int totalSamples = 0;
    for (auto it = m_renderData.constBegin(); it != m_renderData.constEnd(); ++it) {
        totalSamples += it.value().size();
    }
    
    // 状态信息使用小字体
    QFont statusFont = m_overlayFont;
    statusFont.setPointSize(m_overlayFont.pointSize() - 1);
    painter->setFont(statusFont);
    
    // 显示性能监控状态信息
    painter->drawText(textX + 5, textY, 
                    QString("采样: %1  |  指标: %2  |  线程: %3")
                    .arg(m_worker->getMaxSamples())
                    .arg(m_renderData.size())
                    .arg(m_workerThread && m_workerThread->isRunning() ? "运行中" : "已停止"));
    
    // 恢复字体
    painter->setFont(m_overlayFont);
}

// 获取指标名称
QString PerformanceMonitor::getMetricName(MetricType type) const
{
    switch (type) {
        case FrameTime:
            return "帧时间";
        case UpdateTime:
            return "更新时间";
        case EventTime:
            return "事件处理";
        case DrawTime:
            return "绘制时间";
        case LogicTime:
            return "逻辑处理";
        case ShapesDrawTime:
            return "图形绘制";
        case PathProcessTime:
            return "路径处理";
        case RenderPrepTime:
            return "渲染准备";
        case CpuUsage:
            return "CPU使用率";
        case MemoryUsage:
            return "内存使用";
        case ThreadCount:
            return "线程数量";
        case CustomMetric1:
            if (m_worker) {
                QString customName = m_worker->getCustomName(CustomMetric1);
                if (!customName.isEmpty()) {
                    return customName;
                }
            }
            return "自定义1";
        case CustomMetric2:
            if (m_worker) {
                QString customName = m_worker->getCustomName(CustomMetric2);
                if (!customName.isEmpty()) {
                    return customName;
                }
            }
            return "自定义2";
        default:
            return QString("未知指标-%1").arg(type);
    }
}

// 获取指标颜色
QColor PerformanceMonitor::getMetricColor(MetricType type) const
{
    switch (type) {
        case FrameTime:
            return QColor(255, 100, 100);  // 红色
        case UpdateTime:
            return QColor(100, 255, 100);  // 绿色
        case EventTime:
            return QColor(100, 100, 255);  // 蓝色
        case DrawTime:
            return QColor(255, 255, 100);  // 黄色
        case LogicTime:
            return QColor(255, 100, 255);  // 紫色
        case ShapesDrawTime:
            return QColor(100, 255, 255);  // 青色
        case PathProcessTime:
            return QColor(255, 180, 100);  // 橙色
        case RenderPrepTime:
            return QColor(180, 255, 180);  // 淡绿色
        case CpuUsage:
            return QColor(255, 120, 0);    // 深橙色
        case MemoryUsage:
            return QColor(0, 180, 255);    // 蓝绿色
        case ThreadCount:
            return QColor(180, 120, 255);  // 紫罗兰色
        case CustomMetric1:
            return QColor(220, 180, 0);    // 金色
        case CustomMetric2:
            return QColor(0, 180, 120);    // 碧绿色
        default:
            return QColor(200, 200, 200);  // 灰色
    }
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
    
    // 更新计时器间隔，保持频繁检查
    if (m_processTimer && !events.isEmpty()) {
        // 如果刚处理了事件，缩短下次检查间隔，以便更快处理后续事件
        int currentInterval = m_processTimer->interval();
        if (currentInterval > 5) {  // 最小间隔为5ms
            m_processTimer->setInterval(5);
        }
    }
}

// 添加新方法：立即处理队列中的所有事件（用于同步点）
void PerformanceMonitor::flushEvents()
{
    checkQueue();
    
    // 给工作线程一些时间处理事件
    QThread::msleep(5);
    
    // 重置计时器间隔
    if (m_processTimer) {
        m_processTimer->setInterval(m_processingInterval);
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
    
    // 特殊情况：对于结束测量事件，尝试立即处理队列
    if (event.type == EVENT_END_MEASURE) {
        locker.unlock();
        checkQueue();
    }
}

// 设置图表显示模式
void PerformanceMonitor::setChartDisplayMode(ChartDisplayMode mode)
{
    if (m_chartMode != mode) {
        m_chartMode = mode;
        Logger::debug(QString("性能监控图表模式已变更为: %1").arg(mode));
    }
}

// 获取当前图表显示模式
PerformanceMonitor::ChartDisplayMode PerformanceMonitor::getChartDisplayMode() const
{
    return m_chartMode;
}

// 设置覆盖层位置
void PerformanceMonitor::setOverlayPosition(OverlayPosition position)
{
    if (m_overlayPosition != position) {
        m_overlayPosition = position;
        Logger::debug(QString("性能监控覆盖层位置已变更为: %1").arg(position));
    }
}

// 获取覆盖层位置
PerformanceMonitor::OverlayPosition PerformanceMonitor::getOverlayPosition() const
{
    return m_overlayPosition;
}

// 设置覆盖层透明度
void PerformanceMonitor::setOverlayOpacity(double opacity)
{
    // 限制透明度范围
    opacity = qBound(0.1, opacity, 1.0);
    
    if (!qFuzzyCompare(m_overlayOpacity, opacity)) {
        m_overlayOpacity = opacity;
        Logger::debug(QString("性能监控覆盖层透明度已变更为: %1").arg(opacity));
    }
}

// 获取覆盖层透明度
double PerformanceMonitor::getOverlayOpacity() const
{
    return m_overlayOpacity;
}

// 设置覆盖层大小
void PerformanceMonitor::setOverlaySize(int width, int height)
{
    // 限制尺寸范围，避免过大或过小
    width = qBound(200, width, 800);
    height = qBound(150, height, 600);
    
    if (m_overlayWidth != width || m_overlayHeight != height) {
        m_overlayWidth = width;
        m_overlayHeight = height;
        Logger::debug(QString("性能监控覆盖层尺寸已变更为: %1x%2").arg(width).arg(height));
    }
}

// 计算覆盖层在指定视口中的位置
QRectF PerformanceMonitor::calculateOverlayRect(const QRectF& viewportRect) const
{
    // 根据设置的位置计算覆盖层的实际位置
    QRectF result;
    const int margin = 10; // 边缘距离
    
    switch (m_overlayPosition) {
        case TopLeft:
            result = QRectF(viewportRect.left() + margin, 
                           viewportRect.top() + margin, 
                           m_overlayWidth, m_overlayHeight);
            break;
            
        case TopRight:
            result = QRectF(viewportRect.right() - m_overlayWidth - margin, 
                           viewportRect.top() + margin, 
                           m_overlayWidth, m_overlayHeight);
            break;
            
        case BottomLeft:
            result = QRectF(viewportRect.left() + margin, 
                           viewportRect.bottom() - m_overlayHeight - margin, 
                           m_overlayWidth, m_overlayHeight);
            break;
            
        case BottomRight:
            result = QRectF(viewportRect.right() - m_overlayWidth - margin, 
                           viewportRect.bottom() - m_overlayHeight - margin, 
                           m_overlayWidth, m_overlayHeight);
            break;
    }
    
    // 确保不会超出视口范围
    if (result.right() > viewportRect.right() - margin/2) {
        result.moveRight(viewportRect.right() - margin/2);
    }
    if (result.bottom() > viewportRect.bottom() - margin/2) {
        result.moveBottom(viewportRect.bottom() - margin/2);
    }
    if (result.left() < viewportRect.left() + margin/2) {
        result.moveLeft(viewportRect.left() + margin/2);
    }
    if (result.top() < viewportRect.top() + margin/2) {
        result.moveTop(viewportRect.top() + margin/2);
    }
    
    return result;
}

// 辅助方法：绘制图表网格线
void PerformanceMonitor::drawChartGrid(QPainter* painter, const QRectF& rect, qint64 maxVal)
{
    // 绘制图表背景
    QLinearGradient bgGradient(rect.topLeft(), rect.bottomLeft());
    bgGradient.setColorAt(0, QColor(40, 40, 50, 220));
    bgGradient.setColorAt(1, QColor(20, 20, 30, 220));
    painter->fillRect(rect, bgGradient);
    
    // 绘制边框
    painter->setPen(QPen(QColor(100, 100, 120, 180), 1));
    painter->drawRect(rect);
    
    // 绘制水平网格线和标签
    painter->setFont(QFont("Arial", 7));
    QFontMetrics fm(painter->font());
    
    int steps = 5;  // 分成5个刻度
    for (int i = 0; i <= steps; i++) {
        qreal y = rect.top() + (rect.height() * (steps - i) / steps);
        qint64 value = maxVal * i / steps;
        
        // 绘制网格线
        painter->setPen(QPen(QColor(80, 80, 90, 120), 1, Qt::DotLine));
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
        
        // 绘制刻度标签
        painter->setPen(QColor(200, 200, 200));
        QString label = QString::number(value) + "ms";
        QRectF textRect(rect.left() - fm.horizontalAdvance(label) - 5, y - fm.height()/2, 
                      fm.horizontalAdvance(label), fm.height());
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }
    
    // 绘制垂直网格线
    int timeSteps = 4;  // 每个图表宽度分为4个时间间隔
    for (int i = 1; i < timeSteps; i++) {
        qreal x = rect.left() + (rect.width() * i) / timeSteps;
        
        // 绘制垂直辅助线
        painter->setPen(QPen(QColor(80, 80, 90, 100), 1, Qt::DotLine));
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
}

// 辅助方法：绘制图表图例
void PerformanceMonitor::drawChartLegend(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data)
{
    // 绘制图例
    int legendItems = data.size();
    int itemHeight = 15;
    int legendPadding = 8;
    QRectF legendRect(
        rect.right() - 100, 
        rect.top() + 5, 
        95, 
        legendItems * itemHeight + legendPadding * 2
    );
    
    QPainterPath legendPath;
    legendPath.addRoundedRect(legendRect, 5, 5);
    
    // 使用半透明背景
    painter->fillPath(legendPath, QColor(0, 0, 0, 150));
    painter->setPen(QColor(100, 100, 100));
    painter->drawPath(legendPath);
    
    // 绘制图例项
    int legendY = legendRect.top() + legendPadding + itemHeight/2;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        MetricType type = it.key();
        QColor color = getMetricColor(type);
        
        // 使用小圆点而不是方块，更美观
        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(QPointF(legendRect.left() + 10, legendY), 4, 4);
        
        // 绘制指标名称
        painter->setPen(QColor(220, 220, 220));
        painter->drawText(QPointF(legendRect.left() + 20, legendY + 4), getMetricName(type));
        
        legendY += itemHeight;
    }
}

// 柱状图渲染
void PerformanceMonitor::renderBarChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal)
{
    QRectF graphRect = rect;
    
    // 绘制网格线
    drawChartGrid(painter, graphRect, maxVal);
    
    // 图例
    drawChartLegend(painter, graphRect, data);
    
    // 检查是否有有效数据
    bool hasValidData = false;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        if (!it.value().isEmpty()) {
            hasValidData = true;
            break;
        }
    }
    
    if (!hasValidData) {
        // 没有数据时显示提示信息
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(graphRect, Qt::AlignCenter, "等待数据...");
        return;
    }
    
    // 确定显示的样本数量和每组柱的宽度
    const int maxSamples = 20; // 最多显示20个样本点，避免过度拥挤
    
    // 对于每个指标，绘制柱状图
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        MetricType type = it.key();
        QList<qint64> measurements = it.value();
        
        if (measurements.isEmpty()) continue;
        
        // 只有一个数据点的特殊处理
        if (measurements.size() == 1) {
            QColor barColor = getMetricColor(type);
            painter->setBrush(barColor);
            painter->setPen(QPen(barColor.darker(120), 1));
            
            // 居中绘制一个柱
            qreal barWidth = graphRect.width() / 3;
            qreal x = graphRect.center().x() - barWidth / 2;
            qreal valueRatio = qMin(1.0, measurements[0] / static_cast<double>(maxVal));
            qreal barHeight = valueRatio * graphRect.height();
            
            QRectF barRect(x, graphRect.bottom() - barHeight, barWidth, barHeight);
            painter->drawRect(barRect);
            
            // 绘制数值标签
            painter->setPen(QColor(220, 220, 220));
            QString valueText = QString::number(measurements[0]) + "ms";
            painter->drawText(
                QPointF(barRect.center().x() - painter->fontMetrics().horizontalAdvance(valueText) / 2, 
                        barRect.top() - 5), 
                valueText
            );
            
            continue;
        }
        
        // 取最近的样本
        QList<qint64> displayMeasurements;
        if (measurements.size() > maxSamples) {
            int step = measurements.size() / maxSamples;
            for (int i = measurements.size() - 1; i >= 0 && displayMeasurements.size() < maxSamples; i -= step) {
                displayMeasurements.prepend(measurements.at(i));
            }
        } else {
            displayMeasurements = measurements;
        }
        
        QColor barColor = getMetricColor(type);
        painter->setBrush(barColor);
        painter->setPen(QPen(barColor.darker(120), 1));
        
        // 计算柱宽和间距
        qreal totalWidth = graphRect.width();
        qreal barWidth = totalWidth / (displayMeasurements.size() * 1.5); // 1.5倍留间距
        
        // 绘制柱状
        for (int i = 0; i < displayMeasurements.size(); i++) {
            qreal valueRatio = qMin(1.0, displayMeasurements[i] / static_cast<double>(maxVal));
            qreal barHeight = valueRatio * graphRect.height();
            
            qreal x = graphRect.left() + i * (totalWidth / displayMeasurements.size());
            x += (totalWidth / displayMeasurements.size() - barWidth) / 2; // 居中
            
            QRectF barRect(x, graphRect.bottom() - barHeight, barWidth, barHeight);
            painter->drawRect(barRect);
            
            // 仅在数据点较少时显示数值
            if (displayMeasurements.size() <= 10 && displayMeasurements[i] > 0) {
                painter->setPen(QColor(220, 220, 220));
                QString valueText = QString::number(displayMeasurements[i]);
                painter->drawText(
                    QPointF(barRect.center().x() - painter->fontMetrics().horizontalAdvance(valueText) / 2, 
                            barRect.top() - 5), 
                    valueText
                );
                painter->setPen(QPen(barColor.darker(120), 1));
            }
        }
    }
    
    // 添加当前时间
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("hh:mm:ss");
    painter->setPen(QColor(200, 200, 200));
    QFontMetrics fm(painter->font());
    painter->drawText(
        graphRect.right() - fm.horizontalAdvance(timeStr) - 5, 
        graphRect.bottom() + fm.height() + 2, 
        timeStr
    );
}

// 面积图渲染
void PerformanceMonitor::renderAreaChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal)
{
    QRectF graphRect = rect;
    
    // 绘制网格线
    drawChartGrid(painter, graphRect, maxVal);
    
    // 图例
    drawChartLegend(painter, graphRect, data);
    
    // 收集所有显示指标的数据点，按照指标优先级排序
    QList<MetricType> orderedTypes;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        if (!it.value().isEmpty()) {
            orderedTypes.append(it.key());
        }
    }
    
    // 根据优先级进行排序（面积图中先绘制的会在底层）
    std::sort(orderedTypes.begin(), orderedTypes.end(), [](MetricType a, MetricType b) {
        // 自定义优先级：FrameTime > DrawTime > UpdateTime > EventTime > LogicTime > Custom
        static const QMap<MetricType, int> priorities = {
            {FrameTime, 5},
            {DrawTime, 4},
            {UpdateTime, 3},
            {EventTime, 2},
            {LogicTime, 1},
            {CustomMetric1, 0},
            {CustomMetric2, 0}
        };
        return priorities.value(a) > priorities.value(b);
    });
    
    // 逆序绘制，保证优先级高的在上层
    for (int i = orderedTypes.size() - 1; i >= 0; i--) {
        MetricType type = orderedTypes[i];
        QList<qint64> measurements = data.value(type);
        
        if (measurements.size() > 1) {
            QColor areaColor = getMetricColor(type);
            
            // 计算点间距
            qreal dx = graphRect.width() / (measurements.size() - 1);
            
            // 创建填充路径
            QPainterPath fillPath;
            fillPath.moveTo(graphRect.left(), graphRect.bottom());
            
            // 添加所有数据点
            for (int j = 0; j < measurements.size(); j++) {
                qreal x = graphRect.left() + j * dx;
                qreal valueRatio = qMin(1.0, measurements[j] / static_cast<double>(maxVal));
                qreal y = graphRect.bottom() - (valueRatio * graphRect.height());
                
                fillPath.lineTo(x, y);
            }
            
            // 闭合路径
            fillPath.lineTo(graphRect.right(), graphRect.bottom());
            fillPath.closeSubpath();
            
            // 设置半透明填充，较低的透明度让下层可见
            QColor fillColor = areaColor;
            fillColor.setAlpha(100);
            painter->setBrush(fillColor);
            painter->setPen(Qt::NoPen);
            painter->drawPath(fillPath);
            
            // 绘制顶部边缘线
            QPainterPath linePath;
            bool first = true;
            
            for (int j = 0; j < measurements.size(); j++) {
                qreal x = graphRect.left() + j * dx;
                qreal valueRatio = qMin(1.0, measurements[j] / static_cast<double>(maxVal));
                qreal y = graphRect.bottom() - (valueRatio * graphRect.height());
                
                if (first) {
                    linePath.moveTo(x, y);
                    first = false;
                } else {
                    linePath.lineTo(x, y);
                }
            }
            
            painter->setPen(QPen(areaColor, 2));
            painter->drawPath(linePath);
        }
    }
    
    // 添加当前时间
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("hh:mm:ss");
    painter->setPen(QColor(200, 200, 200));
    QFontMetrics fm(painter->font());
    painter->drawText(
        graphRect.right() - fm.horizontalAdvance(timeStr) - 5, 
        graphRect.bottom() + fm.height() + 2, 
        timeStr
    );
}

// 点状图渲染
void PerformanceMonitor::renderDotChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal)
{
    QRectF graphRect = rect;
    
    // 绘制网格线
    drawChartGrid(painter, graphRect, maxVal);
    
    // 图例
    drawChartLegend(painter, graphRect, data);
    
    // 对于每个指标，绘制散点
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        MetricType type = it.key();
        QList<qint64> measurements = it.value();
        
        if (measurements.isEmpty()) continue;
        
        QColor dotColor = getMetricColor(type);
        painter->setBrush(dotColor);
        
        // 计算点间距
        qreal dx = graphRect.width() / (measurements.size() - 1);
        
        // 绘制所有数据点
        for (int i = 0; i < measurements.size(); i++) {
            qreal x = graphRect.left() + i * dx;
            qreal valueRatio = qMin(1.0, measurements[i] / static_cast<double>(maxVal));
            qreal y = graphRect.bottom() - (valueRatio * graphRect.height());
            
            // 绘制数据点
            painter->setPen(QPen(dotColor.darker(120), 1));
            painter->drawEllipse(QPointF(x, y), 3, 3);
            
            // 添加垂直线连接到底部，增强可读性
            if (i % 3 == 0) { // 每3个点绘制一条垂直线
                painter->setPen(QPen(dotColor, 0.5, Qt::DotLine));
                painter->drawLine(QPointF(x, y), QPointF(x, graphRect.bottom()));
            }
        }
    }
    
    // 添加当前时间
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("hh:mm:ss");
    painter->setPen(QColor(200, 200, 200));
    QFontMetrics fm(painter->font());
    painter->drawText(
        graphRect.right() - fm.horizontalAdvance(timeStr) - 5, 
        graphRect.bottom() + fm.height() + 2, 
        timeStr
    );
}

// 注册自定义指标回调
void PerformanceMonitor::registerMetricCallback(const QString& name, std::function<void(QMap<QString, QVariant>&)> callback)
{
    if (name.isEmpty() || !callback) return;
    
    QMutexLocker locker(&m_queueMutex); // 保护回调映射
    m_metricCallbacks[name] = callback;
    Logger::debug(QString("已注册性能指标回调: %1").arg(name));
}

// 获取自定义事件数据
QMap<QString, QList<qint64>> PerformanceMonitor::getCustomEventData() const
{
    if (!m_enabled.load() || !m_worker) return {};
    return m_worker->getCustomEventData();
}

// 重置性能数据
void PerformanceMonitor::resetData()
{
    if (!m_enabled.load()) return;
    
    PerformanceEvent event(EVENT_RESET_DATA);
    enqueueEvent(event);
}

// PerformanceWorker methods
void PerformanceWorker::recordCustomEvent(const QString& name, qint64 value)
{
    if (name.isEmpty()) return;
    
    QWriteLocker locker(&m_dataLock);
    recordCustomMeasurement(name, value);
}

void PerformanceWorker::recordCustomMeasurement(const QString& name, qint64 value)
{
    // 确保在锁内调用
    
    // 创建条目（如果不存在）
    if (!m_customEvents.contains(name)) {
        m_customEvents[name] = QList<qint64>();
    }
    
    // 添加数据并限制大小
    m_customEvents[name].append(value);
    
    // 裁剪数据保持在最大样本数内
    while (m_customEvents[name].size() > m_maxSamples) {
        m_customEvents[name].removeFirst();
    }
    
    // 通知数据已更新
    emit measurementsUpdated();
}

QMap<QString, QList<qint64>> PerformanceWorker::getCustomEventData() const
{
    QReadLocker locker(&m_dataLock);
    return m_customEvents;
}

// 系统资源监控相关实现 ===================================

// CPU使用率监控
void PerformanceMonitor::monitorCpuUsage()
{
    // 使用QProcess获取系统CPU使用情况
    static qint64 lastTotalUser = 0;
    static qint64 lastTotalSystem = 0;
    static qint64 lastTotalIdle = 0;
    
#ifdef Q_OS_WIN
    QProcess process;
    process.start("wmic", QStringList() << "cpu" << "get" << "loadpercentage");
    process.waitForFinished(1000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    
    // 解析输出，提取CPU负载百分比
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    if (lines.size() >= 2) {
        bool ok;
        int cpuLoad = lines[1].trimmed().toInt(&ok);
        if (ok) {
            recordEvent("CpuUsage", cpuLoad);
            startMeasure(CpuUsage);
            endMeasure(CpuUsage);
        }
    }
#else
    // Linux/macOS实现
    QFile file("/proc/stat");
    if (file.open(QIODevice::ReadOnly)) {
        QString line = file.readLine();
        file.close();
        
        QStringList values = line.split(" ", Qt::SkipEmptyParts);
        if (values.size() >= 5) {
            qint64 user = values[1].toLongLong();
            qint64 nice = values[2].toLongLong();
            qint64 system = values[3].toLongLong();
            qint64 idle = values[4].toLongLong();
            
            qint64 totalUser = user + nice;
            qint64 totalSystem = system;
            qint64 totalIdle = idle;
            
            qint64 total = totalUser + totalSystem + totalIdle;
            
            if (lastTotalUser > 0 && lastTotalSystem > 0 && lastTotalIdle > 0) {
                qint64 diffTotal = total - (lastTotalUser + lastTotalSystem + lastTotalIdle);
                qint64 diffIdle = totalIdle - lastTotalIdle;
                
                if (diffTotal > 0) {
                    qint64 cpuUsage = 100 * (diffTotal - diffIdle) / diffTotal;
                    recordEvent("CpuUsage", cpuUsage);
                    startMeasure(CpuUsage);
                    endMeasure(CpuUsage);
                }
            }
            
            lastTotalUser = totalUser;
            lastTotalSystem = totalSystem;
            lastTotalIdle = totalIdle;
        }
    }
#endif
}

// 内存使用监控
void PerformanceMonitor::monitorMemoryUsage()
{
#ifdef Q_OS_WIN
    QProcess process;
    process.start("wmic", QStringList() << "OS" << "get" << "FreePhysicalMemory,TotalVisibleMemorySize");
    process.waitForFinished(1000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    if (lines.size() >= 2) {
        QStringList values = lines[1].trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (values.size() >= 2) {
            bool ok1, ok2;
            qint64 freeMemory = values[0].toLongLong(&ok1) * 1024; // KB to bytes
            qint64 totalMemory = values[1].toLongLong(&ok2) * 1024; // KB to bytes
            
            if (ok1 && ok2 && totalMemory > 0) {
                qint64 usedMemory = totalMemory - freeMemory;
                int memoryPercentage = (usedMemory * 100) / totalMemory;
                
                recordEvent("MemoryUsageBytes", usedMemory);
                recordEvent("MemoryUsagePercent", memoryPercentage);
                
                // 使用内置度量类型记录
                startMeasure(MemoryUsage);
                endMeasure(MemoryUsage);
            }
        }
    }
    
    // 获取当前进程内存使用情况
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        qint64 appMemoryUsage = pmc.WorkingSetSize;
        recordEvent("AppMemoryUsage", appMemoryUsage);
    }
#else
    // Linux/macOS实现
    QProcess process;
    process.start("free", QStringList() << "-b");
    process.waitForFinished(1000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    if (lines.size() >= 2) {
        QStringList values = lines[1].trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (values.size() >= 3) {
            bool ok1, ok2;
            qint64 totalMemory = values[1].toLongLong(&ok1);
            qint64 usedMemory = values[2].toLongLong(&ok2);
            
            if (ok1 && ok2 && totalMemory > 0) {
                int memoryPercentage = (usedMemory * 100) / totalMemory;
                
                recordEvent("MemoryUsageBytes", usedMemory);
                recordEvent("MemoryUsagePercent", memoryPercentage);
                
                // 使用内置度量类型记录
                startMeasure(MemoryUsage);
                endMeasure(MemoryUsage);
            }
        }
    }
    
    // 获取当前进程内存使用情况
    QProcess appProcess;
    QString pid = QString::number(QCoreApplication::applicationPid());
    appProcess.start("ps", QStringList() << "-p" << pid << "-o" << "rss=");
    appProcess.waitForFinished(1000);
    QString appOutput = QString::fromLocal8Bit(appProcess.readAllStandardOutput()).trimmed();
    
    bool ok;
    qint64 appMemoryUsage = appOutput.toLongLong(&ok) * 1024; // KB to bytes
    if (ok) {
        recordEvent("AppMemoryUsage", appMemoryUsage);
    }
#endif
}

// 线程数量监控
void PerformanceMonitor::monitorThreadCount()
{
#ifdef Q_OS_WIN
    DWORD pid = GetCurrentProcessId();
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te;
        te.dwSize = sizeof(THREADENTRY32);
        int threadCount = 0;
        
        if (Thread32First(hSnapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == pid) {
                    threadCount++;
                }
            } while (Thread32Next(hSnapshot, &te));
        }
        
        CloseHandle(hSnapshot);
        
        recordEvent("ThreadCount", threadCount);
        startMeasure(ThreadCount);
        endMeasure(ThreadCount);
    }
#else
    // Linux/macOS实现
    QProcess process;
    QString pid = QString::number(QCoreApplication::applicationPid());
    process.start("ps", QStringList() << "-L" << "-p" << pid << "--no-headers" << "-o" << "nlwp");
    process.waitForFinished(1000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    
    bool ok;
    int threadCount = output.toInt(&ok);
    if (ok) {
        recordEvent("ThreadCount", threadCount);
        startMeasure(ThreadCount);
        endMeasure(ThreadCount);
    }
#endif
}

// 初始化系统资源监控
void PerformanceMonitor::initializeResourceMonitoring()
{
    // 创建资源监控定时器
    QTimer* resourceTimer = new QTimer(this);
    resourceTimer->setInterval(1000); // 每秒更新一次
    
    // 定时器触发时执行资源监控
    connect(resourceTimer, &QTimer::timeout, this, [this]() {
        if (m_enabled.load() && (m_visibleMetrics.contains(CpuUsage) || 
                               m_visibleMetrics.contains(MemoryUsage) || 
                               m_visibleMetrics.contains(ThreadCount))) {
            monitorCpuUsage();
            monitorMemoryUsage();
            monitorThreadCount();
        }
    });
    
    // 性能监控启用/禁用时自动控制资源监控
    connect(this, &PerformanceMonitor::enabledChanged, resourceTimer, [resourceTimer](bool enabled) {
        if (enabled) {
            resourceTimer->start();
        } else {
            resourceTimer->stop();
        }
    });
    
    // 如果当前已启用性能监控，启动资源监控
    if (m_enabled.load()) {
        resourceTimer->start();
    }
    
    // 注册系统资源回调
    registerMetricCallback("SystemResources", [this](QMap<QString, QVariant>& data) {
        // CPU使用数据
        QList<qint64> cpuData = m_worker->getMeasurementData(CpuUsage);
        if (!cpuData.isEmpty()) {
            data["CpuUsage"] = cpuData.last();
            data["CpuUsageAvg"] = calculateAverage(cpuData);
        }
        
        // 内存使用数据
        QList<qint64> memData = m_worker->getMeasurementData(MemoryUsage);
        QMap<QString, QList<qint64>> customEvents = m_worker->getCustomEventData();
        
        if (customEvents.contains("MemoryUsageBytes")) {
            QList<qint64> memBytes = customEvents["MemoryUsageBytes"];
            if (!memBytes.isEmpty()) {
                data["MemoryBytes"] = memBytes.last();
            }
        }
        
        if (customEvents.contains("MemoryUsagePercent")) {
            QList<qint64> memPercent = customEvents["MemoryUsagePercent"];
            if (!memPercent.isEmpty()) {
                data["MemoryPercent"] = memPercent.last();
            }
        }
        
        // 线程数据
        QList<qint64> threadData = m_worker->getMeasurementData(ThreadCount);
        if (!threadData.isEmpty()) {
            data["ThreadCount"] = threadData.last();
        }
        
        // 应用内存使用
        if (customEvents.contains("AppMemoryUsage")) {
            QList<qint64> appMemUsage = customEvents["AppMemoryUsage"];
            if (!appMemUsage.isEmpty()) {
                // 转换为MB便于显示
                double mbUsage = appMemUsage.last() / (1024.0 * 1024.0);
                data["AppMemoryMB"] = mbUsage;
            }
        }
    });
}

// 计算平均值
double PerformanceMonitor::calculateAverage(const QList<qint64>& measurements) const
{
    if (measurements.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (qint64 val : measurements) {
        sum += val;
    }
    
    return sum / measurements.size();
}

// 数据同步方法
void PerformanceMonitor::ensureDataSynced() const
{
    QMutexLocker renderLocker(&m_renderLock);
    
    // 检查是否需要同步数据
    QDateTime now = QDateTime::currentDateTime();
    if (!m_lastDataSync.isValid() || m_lastDataSync.msecsTo(now) > 100) { // 至少100ms同步一次
        if (m_worker) {
            // 获取工作线程数据的安全副本
            QMap<int, QList<qint64>> measurements = m_worker->getMeasurementsCopy();
            
            // 更新渲染数据
            m_renderData.clear();
            for (auto it = measurements.constBegin(); it != measurements.constEnd(); ++it) {
                if (!it.value().isEmpty() && m_visibleMetrics.contains(static_cast<MetricType>(it.key()))) {
                    m_renderData[static_cast<MetricType>(it.key())] = it.value();
                }
            }
            
            m_lastDataSync = now;
        }
    }
}
