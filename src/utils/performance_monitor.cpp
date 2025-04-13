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
    
    // 使用原子操作增加帧计数器
    int currentFrameCount = ++m_frameCount;
    
    // 避免帧数异常
    if (currentFrameCount > 10000) {
        // 使用原子操作重置计数器
        m_frameCount.store(1);
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
        int newFPS = currentFrameCount * 1000 / elapsed;
        
        // 限制FPS范围，避免异常值
        if (newFPS < 0) newFPS = 0;
        if (newFPS > 1000) newFPS = 1000;
        
        m_currentFPS.store(newFPS);
        m_frameCount.store(0);
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
        Logger::warning(QString("性能监测值异常: %1 ms, 已自动调整到有效范围").arg(elapsedMs));
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
    
    // 减少日志输出频率，提高性能
    if (samples.size() == 1 || samples.size() % 10 == 0) {
        Logger::debug(QString("记录性能指标: 类型=%1, 值=%2ms, 样本数=%3")
                      .arg(type)
                      .arg(elapsedMs)
                      .arg(samples.size()));
        
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

// 获取自定义事件数据
QMap<QString, QList<qint64>> PerformanceWorker::getCustomEventData() const
{
    QReadLocker locker(&m_dataLock);
    return m_customEvents;
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

// 记录自定义测量
void PerformanceWorker::recordCustomMeasurement(const QString& name, qint64 value)
{
    if (!m_enabled || name.isEmpty()) return;
    
    QWriteLocker locker(&m_dataLock);
    
    // 检查是否已经存在该名称的测量容器，如果没有则创建
    if (!m_customEvents.contains(name)) {
        m_customEvents[name] = QList<qint64>();
        // 预分配空间，提高性能
        m_customEvents[name].reserve(m_maxSamples);
    }
    
    // 添加测量值
    m_customEvents[name].append(value);
    
    // 限制样本数量
    auto& samples = m_customEvents[name];
    if (samples.size() > m_maxSamples) {
        samples.removeFirst();
    }
    
    Logger::debug(QString("记录自定义事件: 名称=%1, 值=%2, 样本数=%3")
                 .arg(name)
                 .arg(value)
                 .arg(samples.size()));
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
    static QMutex instanceMutex;
    QMutexLocker locker(&instanceMutex);
    return s_instancePtr != nullptr;
}

// 构造函数
PerformanceMonitor::PerformanceMonitor(QObject* parent)
    : QObject(parent)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_processTimer(nullptr)
    , m_enabled(false)
{    
    // 默认启用帧率和绘制时间监控
    m_visibleMetrics.insert(FrameTime);
    m_visibleMetrics.insert(DrawTime);
    
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
        
        // 创建和连接处理定时器
        m_processTimer = new QTimer(this);
        m_processTimer->setInterval(m_processingInterval);
        connect(m_processTimer, &QTimer::timeout, this, &PerformanceMonitor::checkQueue);
        
        // 启动工作线程和定时器
        m_workerThread->start();
        m_processTimer->start();
        
        
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
    m_isShuttingDown.store(true);
    
    if (m_processTimer) {
        m_processTimer->stop();
        delete m_processTimer;
        m_processTimer = nullptr;
    }
    
    if (m_workerThread) {
        m_workerThread->quit();
        if (!m_workerThread->wait(1000)) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
        // worker 对象会在线程结束时自动删除，因为我们使用了 deleteLater
    }
    
    // 清理单例指针
    s_instancePtr = nullptr;
}

// 启用/禁用性能监控
void PerformanceMonitor::setEnabled(bool enabled)
{
    if (m_enabled.load() != enabled) {
        // 首先更新本地状态
        m_enabled.store(enabled);
        
        // 如果是开启状态，记录开始时间
        if (enabled) {
            m_startTime = QDateTime::currentDateTime();
        }
        
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
    
    // 应用概述信息
    stream << "【应用概述】\n";
    stream << "监控运行时长: " << formatDuration(m_startTime.msecsTo(currentDateTime)) << "\n";
    stream << "监控开始时间: " << m_startTime.toString("yyyy-MM-dd hh:mm:ss") << "\n";
    stream << "性能监控状态: " << (m_enabled.load() ? "已启用" : "已禁用") << "\n\n";
    
    // 帧率信息
    int currentFPS = m_worker->getCurrentFPS();
    QList<qint64> frameTimes = m_worker->getMeasurementData(FrameTime);
    double frameTimeAvg = 0.0;
    qint64 frameTimeMax = 0;
    double frameTimeStdDev = 0.0;
    
    if (!frameTimes.isEmpty()) {
        double sum = 0;
        frameTimeMax = frameTimes[0];
        
        for (qint64 value : frameTimes) {
            sum += value;
            if (value > frameTimeMax) frameTimeMax = value;
        }
        
        frameTimeAvg = sum / frameTimes.size();
        
        // 计算帧时间标准差
        double variance = 0.0;
        for (qint64 val : frameTimes) {
            double diff = val - frameTimeAvg;
            variance += diff * diff;
        }
        frameTimeStdDev = frameTimes.size() > 1 ? qSqrt(variance / frameTimes.size()) : 0.0;
    }
    
    stream << "【帧率信息】\n";
    stream << "当前帧率:    " << currentFPS << " FPS\n";
    if (!frameTimes.isEmpty()) {
        stream << "平均帧时间:   " << QString::number(frameTimeAvg, 'f', 2) << " ms\n";
        stream << "最大帧时间:   " << frameTimeMax << " ms\n";
        stream << "帧时间波动:   " << QString::number(frameTimeStdDev, 'f', 2) << " ms\n";
        stream << "帧时间稳定性: " << QString::number(100.0 * (1.0 - qMin(1.0, frameTimeStdDev / (frameTimeAvg + 0.001))), 'f', 1) << "%\n";
        stream << "采样数量:     " << frameTimes.size() << "\n";
        
        // 添加帧率评估
        stream << "帧率评估:     ";
        if (currentFPS >= 60) {
            stream << "流畅 (≥60 FPS)\n";
        } else if (currentFPS >= 30) {
            stream << "良好 (30-59 FPS)\n";
        } else {
            stream << "需要优化 (<30 FPS)\n";
        }
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
    
    // 性能瓶颈检测
    struct MetricStats {
        double avg = 0;
        qint64 max = 0;
        qint64 latest = 0;
        double stdDev = 0;
        double stability = 0;
        int count = 0;
        bool isBottleneck = false;
    };
    
    QMap<int, MetricStats> allMetricsStats;
    double totalAvgTime = 0;
    
    // 输出各类指标数据
    bool hasMetrics = false;
    for (auto it = categories.constBegin(); it != categories.constEnd(); ++it) {
        QList<qint64> measurements = m_worker->getMeasurementData(it.key());
        
        if (!measurements.isEmpty()) {
            MetricStats stats;
            stats.count = measurements.size();
            stats.latest = measurements.last();
            stats.max = measurements[0];
            
            // 计算平均值
            double sum = 0;
            for (qint64 value : measurements) {
                sum += value;
                if (value > stats.max) stats.max = value;
            }
            stats.avg = sum / stats.count;
            
            // 计算标准差
            double variance = 0.0;
            for (qint64 val : measurements) {
                double diff = val - stats.avg;
                variance += diff * diff;
            }
            stats.stdDev = stats.count > 1 ? qSqrt(variance / stats.count) : 0.0;
            
            // 计算稳定性指标
            stats.stability = stats.avg > 0 ? 100.0 * (1.0 - qMin(1.0, stats.stdDev / stats.avg)) : 0.0;
            
            // 存储统计信息以便后续分析
            allMetricsStats[it.key()] = stats;
            
            
            stream << it.value() << "\n";
            stream << "当前值:       " << stats.latest << " ms\n";
            stream << "平均值:       " << QString::number(stats.avg, 'f', 2) << " ms\n";
            stream << "最大值:       " << stats.max << " ms\n";
            stream << "标准差:       " << QString::number(stats.stdDev, 'f', 2) << " ms\n";
            stream << "稳定性指标:   " << QString::number(stats.stability, 'f', 1) << "%\n";
            
            // 添加趋势分析
            if (stats.count >= 10) {
                // 取最后10个样本分析趋势
                QList<qint64> recentMeasurements = measurements.mid(qMax(0, measurements.size() - 10));
                double recentAvg = 0;
                for (qint64 val : recentMeasurements) {
                    recentAvg += val;
                }
                recentAvg /= recentMeasurements.size();
                
                double trend = recentAvg - stats.avg;
                QString trendStr;
                if (qAbs(trend) < 0.1 * stats.avg) {
                    trendStr = "保持稳定";
                } else if (trend > 0) {
                    trendStr = "上升趋势 (+" + QString::number(trend, 'f', 2) + " ms)";
                } else {
                    trendStr = "下降趋势 (" + QString::number(trend, 'f', 2) + " ms)";
                }
                stream << "近期趋势:     " << trendStr << "\n";
            }
            
            stream << "采样数量:     " << stats.count << "\n";
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
            
            // 计算标准差
            double variance = 0.0;
            for (qint64 val : measurements) {
                double diff = val - avg;
                variance += diff * diff;
            }
            double stdDev = measurements.size() > 1 ? qSqrt(variance / measurements.size()) : 0.0;
            
            stream << "【" << name << "】\n";
            stream << "当前值:       " << latest << " ms\n";
            stream << "平均值:       " << QString::number(avg, 'f', 2) << " ms\n";
            stream << "最大值:       " << max << " ms\n";
            stream << "标准差:       " << QString::number(stdDev, 'f', 2) << " ms\n";
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
            
            // 计算标准差
            double variance = 0.0;
            for (qint64 val : data) {
                double diff = val - avg;
                variance += diff * diff;
            }
            double stdDev = data.size() > 1 ? qSqrt(variance / data.size()) : 0.0;
            
            stream << "【" << name << "】\n";
            stream << "当前值:       " << latest << "\n";
            stream << "平均值:       " << QString::number(avg, 'f', 2) << "\n";
            stream << "最大值:       " << max << "\n";
            stream << "标准差:       " << QString::number(stdDev, 'f', 2) << "\n";
            stream << "采样数量:     " << data.size() << "\n";
            stream << "\n";
            
            hasMetrics = true;
        }
    }
    
    if (!hasMetrics) {
        stream << "暂无性能指标数据。使用应用程序一段时间后再查看报告。\n";
    } else {
        // 性能瓶颈分析
        if (totalAvgTime > 0) {
            // 识别性能瓶颈
            for (auto it = allMetricsStats.begin(); it != allMetricsStats.end(); ++it) {
                // 只分析时间类指标（非资源类）
            }
            
            stream << "【性能瓶颈分析】\n";
            bool foundBottleneck = false;
            
            for (auto it = allMetricsStats.begin(); it != allMetricsStats.end(); ++it) {
                if (it.value().isBottleneck) {
                    foundBottleneck = true;
                    double percentage = (it.value().avg / totalAvgTime) * 100.0;
                    stream << getMetricName(static_cast<MetricType>(it.key())) 
                           << ": 占总处理时间的 " << QString::number(percentage, 'f', 1) << "%\n";
                }
            }
            
            if (!foundBottleneck) {
                stream << "未检测到明显的性能瓶颈。\n";
            }
            stream << "\n";
        }
        
        // 性能优化建议
        stream << "【优化建议】\n";
        
        // 基于帧率的建议
        if (currentFPS < 30) {
            stream << "- 帧率较低，建议优化渲染流程和复杂计算。\n";
        }
        
        // 基于绘制时间的建议
        if (allMetricsStats.contains(DrawTime) && allMetricsStats[DrawTime].avg > 16.7) {
            stream << "- 绘制时间超过16.7ms，可能影响流畅度，建议减少绘制复杂度。\n";
        }
        
        // 基于路径处理时间的建议
        if (allMetricsStats.contains(PathProcessTime) && allMetricsStats[PathProcessTime].avg > 5.0) {
            stream << "- 路径处理耗时较长，建议简化复杂路径或使用缓存。\n";
        }
        
        
        // 帧时间波动建议
        if (frameTimeStdDev > 5.0) {
            stream << "- 帧时间波动较大，建议确保计算和渲染负载均衡。\n";
        }
        
        // 如果没有具体建议，提供一般性建议
        if (stream.string()->section("【优化建议】\n", 1, 1).trimmed().isEmpty()) {
            stream << "- 当前性能状况良好，暂无具体优化建议。\n";
            stream << "- 可以考虑启用性能分析器查找更细粒度的优化点。\n";
        }
        
        stream << "\n";
    }
    
    stream << "=======================================\n";
    stream << "性能监控最大样本数: " << m_worker->getMaxSamples() << "\n";
    stream << "报告生成时间: " << currentDateTime.toString("yyyy-MM-dd hh:mm:ss") << "\n";
    
    return report;
}

// 格式化持续时间为可读字符串
QString PerformanceMonitor::formatDuration(qint64 msecs) const
{
    int seconds = msecs / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    minutes %= 60;
    seconds %= 60;
    
    return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
}

// 设置可见指标
void PerformanceMonitor::setVisibleMetrics(const QVector<MetricType>& metrics)
{
    m_visibleMetrics.clear();
    for (const MetricType& metric : metrics) {
        m_visibleMetrics.insert(metric);
    }
}

// 设置采样数
void PerformanceMonitor::setSamplesCount(int samplesCount)
{
    if (!m_worker || samplesCount <= 0) return;
    
    // 使用线程安全方法设置样本数
    m_worker->setMaxSamples(samplesCount);
    
    Logger::info(QString("性能监控：设置采样数为 %1").arg(samplesCount));
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

// 处理队列中的所有事件（清空队列）
void PerformanceMonitor::flushEvents()
{
    checkQueue();
    
    // 给工作线程一些时间处理事件
    QThread::msleep(5);
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

// 注册指标回调函数
void PerformanceMonitor::registerMetricCallback(const QString& name, std::function<void(QMap<QString, QVariant>&)> callback)
{
    if (name.isEmpty() || !callback) {
        Logger::warning("性能监控：尝试注册无效回调");
        return;
    }
    
    m_metricCallbacks[name] = callback;
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
    if (!m_worker) {
        Logger::warning("性能监控：尝试同步数据但工作线程对象为空");
        return;
    }
    
    QMutexLocker renderLocker(&m_renderLock);
    
    // 检查是否需要同步数据
    QDateTime now = QDateTime::currentDateTime();
    if (!m_lastDataSync.isValid() || m_lastDataSync.msecsTo(now) > 100) { // 至少100ms同步一次
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

QMap<QString, QList<qint64>> PerformanceMonitor::getCustomEventData() const
{
    if (!m_enabled.load() || !m_worker) {
        return QMap<QString, QList<qint64>>();
    }
    
    // 确保所有事件已处理
    const_cast<PerformanceMonitor*>(this)->flushEvents();
    
    // 获取工作线程中的自定义事件数据
    return m_worker->getCustomEventData();
}
