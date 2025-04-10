#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QMap>
#include <QString>
#include <QVector>
#include <QMutex>
#include <QPainter>
#include <QColor>
#include <QFont>
#include <QPainterPath>
#include <memory>
#include <QThread>
#include <QQueue>
#include <QTimer>
#include <QWaitCondition>
#include <atomic>
#include <QDateTime>

/**
 * @brief 性能事件类型
 */
enum PerformanceEventType {
    EVENT_START_MEASURE,    // 开始测量
    EVENT_END_MEASURE,      // 结束测量
    EVENT_FRAME_COMPLETED,  // 帧完成
    EVENT_RESET_DATA,       // 重置数据
    EVENT_OVERLAY_TOGGLE,   // 覆盖层开关
    EVENT_CHANGE_ENABLED    // 启用状态变更
};

/**
 * @brief 性能事件结构体
 */
struct PerformanceEvent {
    PerformanceEventType type;               // 事件类型
    int metricType = 0;                      // 指标类型
    qint64 timestamp;                        // 时间戳
    qint64 value = 0;                        // 测量值
    QString customName;                      // 自定义名称
    bool boolValue = false;                  // 布尔值（用于开关类事件）
    
    // 使用静态的QElapsedTimer计时器获取时间戳
    PerformanceEvent(PerformanceEventType t) : type(t) {
        static QElapsedTimer timer;
        if (!timer.isValid()) {
            timer.start();
        }
        timestamp = timer.elapsed();
    }
};

/**
 * @brief 性能监控工作线程类
 */
class PerformanceWorker : public QObject {
    Q_OBJECT
    
public:
    explicit PerformanceWorker(QObject* parent = nullptr);
    ~PerformanceWorker() override;
    
public slots:
    void processEvent(const PerformanceEvent& event);
    void processEvents();
    void stop();
    
signals:
    void measurementsUpdated();
    void statusChanged(bool enabled);
    void overlayToggled(bool enabled);
    
private:
    // 计时器和测量数据
    QMap<int, QElapsedTimer> m_timers;
    QMap<int, QVector<qint64>> m_measurements;
    QMap<int, QString> m_customNames;
    
    // 帧率计算
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;
    int m_currentFPS = 0;
    
    // 内部状态
    int m_maxSamples = 100;
    bool m_enabled = false;
    bool m_overlayEnabled = false;
    
    // 内部处理方法
    void startMeasure(int type, const QString& customName);
    void endMeasure(int type);
    void frameCompleted();
    void resetMeasurements();
    void recordMeasurement(int type, qint64 elapsedMs);
    
    // 计算方法
    double calculateAverage(const QVector<qint64>& measurements) const;
    qint64 calculateMax(const QVector<qint64>& measurements) const;
    
    friend class PerformanceMonitor;
};

/**
 * @brief 性能监控类，用于测量和分析应用的性能指标
 * 
 * 该类使用单例模式，提供统一的性能监控接口。可测量不同类型的操作耗时，
 * 计算帧率，并提供性能数据的可视化显示功能。
 * 
 * 该实现使用异步事件队列设计，避免在UI线程中执行耗时操作。
 */
class PerformanceMonitor : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 性能指标类型枚举
     */
    enum MetricType {
        FrameTime,      // 帧渲染时间
        UpdateTime,     // 场景更新时间
        EventTime,      // 事件处理时间
        DrawTime,       // 绘制时间
        LogicTime,      // 逻辑处理时间
        CustomMetric1,  // 自定义指标1
        CustomMetric2,  // 自定义指标2
        MetricTypeCount // 指标类型总数
    };

    /**
     * @brief 获取单例对象实例
     * @return PerformanceMonitor实例的引用
     */
    static PerformanceMonitor& instance();

    /**
     * @brief 启用或禁用性能监控
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled);

    /**
     * @brief 检查性能监控是否启用
     * @return 是否启用
     */
    bool isEnabled() const;

    /**
     * @brief 开始测量指定类型的操作
     * @param type 指标类型
     * @param customName 自定义名称（仅当使用自定义指标时）
     */
    void startMeasure(MetricType type, const QString& customName = QString());

    /**
     * @brief 结束测量指定类型的操作并记录耗时
     * @param type 指标类型
     */
    void endMeasure(MetricType type);

    /**
     * @brief 自动RAII类，用于测量代码块的执行时间
     */
    class ScopedTimer {
    public:
        /**
         * @brief 构造函数，开始计时
         * @param type 指标类型
         * @param customName 自定义名称
         */
        ScopedTimer(MetricType type, const QString& customName = QString());
        
        /**
         * @brief 析构函数，结束计时
         */
        ~ScopedTimer();
        
    private:
        MetricType m_type;
    };

    /**
     * @brief 更新帧率计数器
     * 应在每帧结束时调用
     */
    void frameCompleted();

    /**
     * @brief 获取当前帧率
     * @return 每秒帧数
     */
    int getFPS() const;

    /**
     * @brief 获取指定指标的平均值
     * @param type 指标类型
     * @return 平均耗时（毫秒）
     */
    double getAverageTime(MetricType type) const;

    /**
     * @brief 获取指定指标的最大值
     * @param type 指标类型
     * @return 最大耗时（毫秒）
     */
    double getMaxTime(MetricType type) const;

    /**
     * @brief 获取性能报告字符串
     * @return 包含所有性能指标的报告
     */
    QString getPerformanceReport() const;

    /**
     * @brief 渲染性能数据到指定绘图设备
     * @param painter 绘图对象
     * @param rect 绘制区域
     */
    void renderOverlay(QPainter* painter, const QRectF& rect);

    /**
     * @brief 设置要显示在覆盖层中的指标
     * @param metrics 要显示的指标类型列表
     */
    void setVisibleMetrics(const QVector<MetricType>& metrics);

    /**
     * @brief 清除所有历史测量数据
     */
    void resetMeasurements();

    /**
     * @brief 设置采样窗口大小
     * @param samplesCount 保留的最近采样数
     */
    void setSamplesCount(int samplesCount);

public slots:
    /**
     * @brief 启用性能覆盖层
     * @param enable 是否启用
     */
    void setOverlayEnabled(bool enable);

    /**
     * @brief 性能覆盖层是否启用
     * @return 是否启用
     */
    bool isOverlayEnabled() const;
    
signals:
    /**
     * @brief 当启用状态改变时发出信号
     */
    void enabledChanged(bool enabled);
    
    /**
     * @brief 当覆盖层状态改变时发出信号
     */
    void overlayEnabledChanged(bool enabled);
    
    /**
     * @brief 当性能数据更新时发出信号
     */
    void dataUpdated();
    
    /**
     * @brief 向工作线程发送事件（内部使用）
     */
    void sendEvent(const PerformanceEvent& event);

private:
    // 单例模式实现
    PerformanceMonitor(QObject* parent = nullptr);
    ~PerformanceMonitor();
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    // 获取指标的可读名称
    QString getMetricName(MetricType type) const;
    
    // 获取指标的颜色
    QColor getMetricColor(MetricType type) const;

    // 渲染性能图表
    void renderPerformanceGraph(QPainter* painter, const QRectF& rect, const QMap<MetricType, QVector<qint64>>& data);

    // 渲染文本信息
    void renderTextInfo(QPainter* painter, const QRectF& rect);
    
    // 将事件添加到队列
    void enqueueEvent(const PerformanceEvent& event);
    
    // 检查是否需要处理事件队列
    void checkQueue();

private:
    // 状态缓存（避免频繁的线程同步）
    std::atomic<bool> m_enabled{false};
    std::atomic<bool> m_overlayEnabled{false};
    
    // 事件队列和同步
    QMutex m_queueMutex;
    QQueue<PerformanceEvent> m_eventQueue;
    QWaitCondition m_queueCondition;
    
    // 工作线程和对象
    QThread* m_workerThread = nullptr;
    PerformanceWorker* m_worker = nullptr;
    
    // 事件处理定时器
    QTimer* m_processTimer = nullptr;
    int m_processingInterval = 16; // ~60fps
    
    // 要显示的指标列表
    QVector<MetricType> m_visibleMetrics;
    
    // 渲染样式
    QFont m_overlayFont;
    int m_graphHeight = 80;
    
    // 渲染数据缓存
    QMutex m_renderMutex;
    QMap<MetricType, QVector<qint64>> m_renderData;
};

// 方便使用的宏
#define PERF_START(metric) PerformanceMonitor::instance().startMeasure(PerformanceMonitor::metric)
#define PERF_END(metric) PerformanceMonitor::instance().endMeasure(PerformanceMonitor::metric)
#define PERF_SCOPE(metric) PerformanceMonitor::ScopedTimer UNIQUE_NAME(perfTimer)(PerformanceMonitor::metric)
#define UNIQUE_NAME(prefix) JOIN(prefix, __LINE__)
#define JOIN(symbol1, symbol2) JOIN_HELPER(symbol1, symbol2)
#define JOIN_HELPER(symbol1, symbol2) symbol1##symbol2

#endif // PERFORMANCE_MONITOR_H 