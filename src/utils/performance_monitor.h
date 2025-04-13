#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QMap>
#include <QString>
#include <QList>
#include <QMutex>
#include <QColor>
#include <QFont>
#include <memory>
#include <QThread>
#include <QQueue>
#include <QTimer>
#include <QWaitCondition>
#include <atomic>
#include <QDateTime>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <functional>
#include <atomic>

/**
 * @brief 性能事件类型
 */
enum PerformanceEventType {
    EVENT_START_MEASURE,    // 开始测量
    EVENT_END_MEASURE,      // 结束测量
    EVENT_FRAME_COMPLETED,  // 帧完成
    EVENT_RESET_DATA,       // 重置数据
    EVENT_CHANGE_ENABLED,   // 启用状态变更
    EVENT_CUSTOM_METRIC     // 自定义指标事件
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
    QVariant customData;                     // 自定义数据
    
    // 使用静态的QElapsedTimer计时器获取时间戳
    PerformanceEvent(PerformanceEventType t) : type(t) {
        static QElapsedTimer timer;
        if (!timer.isValid()) {
            timer.start();
        }
        timestamp = timer.elapsed();
    }
};

// 前向声明
class PerformanceMonitor;

/**
 * @brief 性能监控工作线程类
 */
class PerformanceWorker : public QObject {
    Q_OBJECT
    
public:
    explicit PerformanceWorker(QObject* parent = nullptr);
    ~PerformanceWorker();
    
    // 禁用复制
    PerformanceWorker(const PerformanceWorker&) = delete;
    PerformanceWorker& operator=(const PerformanceWorker&) = delete;
    
    void processEvents(); // 处理事件队列
    int getCurrentFPS() const; // 获取当前帧率
    
    // 数据访问方法
    QMap<int, QList<qint64>> getMeasurementsCopy() const;
    QList<qint64> getMeasurementData(int type) const;
    QString getCustomName(int type) const;
    
    // 设置最大采样数
    void setMaxSamples(int samplesCount);
    int getMaxSamples() const { return m_maxSamples; }
    
    // 获取自定义事件数据
    QMap<QString, QList<qint64>> getCustomEventData() const;
    
public slots:
    void processEvent(const PerformanceEvent& event); // 处理单个事件
    void stop(); // 停止工作

signals:
    void measurementsUpdated();
    void statusChanged(bool enabled);

private:
    // 内部测量方法
    void startMeasure(int type, const QString& customName = QString());
    void endMeasure(int type);
    void frameCompleted();
    void resetMeasurements();
    void recordMeasurement(int type, qint64 elapsedMs);
    void recordCustomMeasurement(const QString& name, qint64 value);
    
    // 工具方法
    double calculateAverage(const QList<qint64>& measurements) const;
    qint64 calculateMax(const QList<qint64>& measurements) const;
    
    // 计时器和测量数据
    mutable QReadWriteLock m_dataLock;     // 数据读写锁，保护测量数据
    QMap<int, QElapsedTimer> m_timers;
    QMap<int, QList<qint64>> m_measurements;
    QMap<int, QString> m_customNames;
    QMap<QString, QList<qint64>> m_customEvents; // 自定义事件存储
    
    // 帧率计算
    QElapsedTimer m_fpsTimer;
    std::atomic<int> m_frameCount; 
    std::atomic<int> m_currentFPS{0};
    
    // 内部状态
    int m_maxSamples = 100;
    std::atomic<bool> m_enabled{false};
    
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
        
        // 图形绘制详细指标
        ShapesDrawTime,     // 图形绘制时间
        PathProcessTime,    // 路径处理时间
        RenderPrepTime,     // 渲染准备时间
        
        // 文件格式操作相关指标
        SaveToCustomFormat, // 保存自定义格式耗时
        LoadFromCustomFormat, // 加载自定义格式耗时
        ExportToSVG,        // 导出SVG耗时
        
        // 其他性能指标
        CacheTime,          // 缓存操作耗时
        ClipTime,           // 裁剪操作耗时
        ResizeTime,         // 调整大小耗时
        PaintTime,          // 绘制耗时
        SelectionTime,      // 选择操作耗时
        CommandTime,        // 命令执行耗时
        NetworkTime,        // 网络操作耗时
        IoTime,             // IO操作耗时
        
        // 自定义指标
        CustomMetric1,      // 自定义指标1
        CustomMetric2,      // 自定义指标2
        MetricTypeCount     // 指标类型总数
    };
    
    /**
     * @brief 性能监控类别枚举
     */
    enum MonitorCategory {
        DrawTimeCategory,
        UpdateTimeCategory,
        EventTimeCategory,
        CacheTimeCategory,
        ClipTimeCategory,
        ResizeTimeCategory,
        PaintTimeCategory,
        SelectionTimeCategory,
        CommandTimeCategory,
        NetworkTimeCategory,
        IoTimeCategory,
        // 添加文件格式操作相关类别
        SaveToCustomFormatCategory,
        LoadFromCustomFormatCategory,
        ExportToSVGCategory,
        // ... 其他可能的类别
        CpuUsageCategory,
        GpuUsageCategory,
        MemoryUsageCategory,
        DiskUsageCategory,
        NetworkUsageCategory,
        TotalCategoriesCount
    };

    /**
     * @brief 获取单例对象实例
     * @return PerformanceMonitor实例的引用
     */
    static PerformanceMonitor& instance();
    
    /**
     * @brief 检查性能监控实例是否已创建，不会触发实例化
     * @return 实例是否已创建
     */
    static bool isInstanceCreated();

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
     * @brief 开始测量指定类型的性能指标
     * @param type 性能指标类型
     * @param customName 自定义指标名称（仅对自定义指标有效）
     */
    void startMeasure(MetricType type, const QString& customName = QString());

    /**
     * @brief 结束测量指定类型的性能指标
     * @param type 性能指标类型
     */
    void endMeasure(MetricType type);

    /**
     * @brief 通知帧完成，用于计算帧率
     */
    void frameCompleted();

    /**
     * @brief 记录自定义事件或指标
     * @param name 事件名称
     * @param value 事件值
     */
    void recordEvent(const QString& name, qint64 value);

    /**
     * @brief 获取当前帧率
     * @return 当前帧率
     */
    int getFPS() const;

    /**
     * @brief 获取指定指标的平均耗时
     * @param type 指标类型
     * @return 平均耗时（毫秒）
     */
    double getAverageTime(MetricType type) const;

    /**
     * @brief 获取指定指标的最大耗时
     * @param type 指标类型
     * @return 最大耗时（毫秒）
     */
    double getMaxTime(MetricType type) const;
    
    /**
     * @brief 确保数据同步
     */
    void ensureDataSynced() const;

    /**
     * @brief 获取自定义事件数据
     * @return 包含自定义事件数据的映射
     */
    QMap<QString, QList<qint64>> getCustomEventData() const;

    /**
     * @brief 生成性能报告文本
     * @return 性能报告文本
     */
    QString getPerformanceReport() const;

    /**
     * @brief 设置可见的指标类型
     * @param metrics 要显示的指标类型列表
     */
    void setVisibleMetrics(const QVector<MetricType>& metrics);
    
    /**
     * @brief 设置性能采样数量
     * @param samplesCount 要保留的采样数量
     */
    void setSamplesCount(int samplesCount);

    /**
     * @brief RAII风格的作用域计时器，自动开始和结束测量
     */
    class ScopedTimer {
    public:
        ScopedTimer(MetricType type, const QString& customName = QString()) 
            : m_type(type), m_finished(false) {
            PerformanceMonitor::instance().startMeasure(m_type, customName);
        }
        
        ~ScopedTimer() {
            if (!m_finished) {
                PerformanceMonitor::instance().endMeasure(m_type);
            }
        }
        
        void finish() {
            if (!m_finished) {
                PerformanceMonitor::instance().endMeasure(m_type);
                m_finished = true;
            }
        }
        
    private:
        MetricType m_type;
        bool m_finished;
    };

    /**
     * @brief 注册指标回调函数，用于收集自定义指标
     * @param name 回调名称
     * @param callback 回调函数
     */
    void registerMetricCallback(const QString& name, std::function<void(QMap<QString, QVariant>&)> callback);

signals:
    /**
     * @brief 性能监控启用状态变更信号
     * @param enabled 是否启用
     */
    void enabledChanged(bool enabled);
    
    /**
     * @brief 数据更新信号
     */
    void dataUpdated();
    
    /**
     * @brief 内部用，向工作线程发送事件的信号
     */
    void sendEvent(const PerformanceEvent& event);

private:
    // 私有构造函数，实现单例模式
    explicit PerformanceMonitor(QObject* parent = nullptr);
    ~PerformanceMonitor() override;
    
    // 禁止拷贝
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    // 单例实例指针
    static PerformanceMonitor* s_instancePtr;
    
    // 工作线程
    QThread* m_workerThread;
    PerformanceWorker* m_worker;
    
    // 性能指标颜色映射
    QMap<MetricType, QColor> m_metricColors;
    
    // 性能指标名称映射
    QMap<MetricType, QString> m_metricNames;
    
    // 可见的指标类型
    QSet<MetricType> m_visibleMetrics;
    
    // 事件队列和互斥锁
    QQueue<PerformanceEvent> m_eventQueue;
    QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    
    // 处理队列的定时器
    QTimer* m_processTimer;
    int m_processingInterval = 50; // ms
    
    // 内部状态
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_isShuttingDown{false};
    std::atomic<bool> m_enabled{false};

    // 监控开始时间
    QDateTime m_startTime;

    // 注册的指标回调函数映射
    QMap<QString, std::function<void(QMap<QString, QVariant>&)>> m_metricCallbacks;
    
    // 渲染相关成员
    mutable QMutex m_renderLock;
    mutable QMap<MetricType, QList<qint64>> m_renderData;
    mutable QDateTime m_lastDataSync;
    
    // 获取指标名称
    QString getMetricName(MetricType type) const;
    
    // 内部方法 - 将事件放入队列
    void enqueueEvent(const PerformanceEvent& event);
    
    // 定时处理队列的槽
    void checkQueue();
    
    // 处理队列中的所有事件（清空队列）
    void flushEvents();
    
    // 格式化持续时间为可读字符串
    QString formatDuration(qint64 msecs) const;
    
    // 计算平均值的辅助方法
    double calculateAverage(const QList<qint64>& measurements) const;
};

// 便捷宏 - 启用条件编译
#ifdef QT_DEBUG
    // 作用域性能测量
    #define PERF_SCOPE(type) PerformanceMonitor::ScopedTimer scopedTimer##__LINE__(PerformanceMonitor::type)
    
    // 开始测量
    #define PERF_START(type) PerformanceMonitor::instance().startMeasure(PerformanceMonitor::type)
    
    // 结束测量
    #define PERF_END(type) PerformanceMonitor::instance().endMeasure(PerformanceMonitor::type)
    
    // 记录自定义事件
    #define PERF_EVENT(name, value) PerformanceMonitor::instance().recordEvent(name, value)
    
    // 帧完成通知
    #define PERF_FRAME_COMPLETED() PerformanceMonitor::instance().frameCompleted()
#else
    // 在非调试版本中，禁用所有宏
    #define PERF_SCOPE(type)
    #define PERF_START(type)
    #define PERF_END(type)
    #define PERF_EVENT(name, value)
    #define PERF_FRAME_COMPLETED()
#endif

#endif // PERFORMANCE_MONITOR_H 