#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QMap>
#include <QString>
#include <QList>
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
#include <QReadWriteLock>
#include <QSharedPointer>
#include <functional>

/**
 * @brief 性能事件类型
 */
enum PerformanceEventType {
    EVENT_START_MEASURE,    // 开始测量
    EVENT_END_MEASURE,      // 结束测量
    EVENT_FRAME_COMPLETED,  // 帧完成
    EVENT_RESET_DATA,       // 重置数据
    EVENT_OVERLAY_TOGGLE,   // 覆盖层开关
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
    ~PerformanceWorker() override;
    
    // 安全获取测量数据的副本
    QMap<int, QList<qint64>> getMeasurementsCopy() const;
    
    // 安全获取帧率
    int getCurrentFPS() const;
    
    // 安全获取单个指标的数据
    QList<qint64> getMeasurementData(int type) const;
    
    // 安全获取自定义名称
    QString getCustomName(int type) const;
    
    // 获取最大样本数
    int getMaxSamples() const { return m_maxSamples; }
    
    // 设置最大样本数并裁剪现有数据
    void setMaxSamples(int samplesCount);
    
    // 记录自定义事件
    void recordCustomEvent(const QString& name, qint64 value);
    
    // 获取自定义事件数据
    QMap<QString, QList<qint64>> getCustomEventData() const;
    
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
    mutable QReadWriteLock m_dataLock;     // 数据读写锁，保护测量数据
    QMap<int, QElapsedTimer> m_timers;
    QMap<int, QList<qint64>> m_measurements;
    QMap<int, QString> m_customNames;
    QMap<QString, QList<qint64>> m_customEvents; // 自定义事件存储
    
    // 帧率计算
    QElapsedTimer m_fpsTimer;
    int m_frameCount = 0;
    std::atomic<int> m_currentFPS{0};
    
    // 内部状态
    int m_maxSamples = 100;
    std::atomic<bool> m_enabled{false};
    std::atomic<bool> m_overlayEnabled{false};
    
    // 内部处理方法
    void startMeasure(int type, const QString& customName);
    void endMeasure(int type);
    void frameCompleted();
    void resetMeasurements();
    void recordMeasurement(int type, qint64 elapsedMs);
    void recordCustomMeasurement(const QString& name, qint64 value);
    
    // 计算方法
    double calculateAverage(const QList<qint64>& measurements) const;
    qint64 calculateMax(const QList<qint64>& measurements) const;
    
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
        
        // 系统资源指标
        CpuUsage,           // CPU使用率
        MemoryUsage,        // 内存使用
        ThreadCount,        // 线程数量
        
        // 自定义指标
        CustomMetric1,      // 自定义指标1
        CustomMetric2,      // 自定义指标2
        MetricTypeCount     // 指标类型总数
    };
    
    /**
     * @brief 性能图表显示模式
     */
    enum ChartDisplayMode {
        LineChart,      // 折线图模式
        BarChart,       // 柱状图模式
        AreaChart,      // 面积图模式
        DotChart        // 点状图模式
    };
    
    /**
     * @brief 覆盖层位置
     */
    enum OverlayPosition {
        TopLeft,        // 左上角
        TopRight,       // 右上角
        BottomLeft,     // 左下角
        BottomRight     // 右下角
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
     * @brief 启用或禁用性能覆盖层
     * @param enable 是否启用
     */
    void setOverlayEnabled(bool enable);

    /**
     * @brief 检查性能覆盖层是否启用
     * @return 是否启用
     */
    bool isOverlayEnabled() const;

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
     * @brief 渲染性能覆盖层
     * @param painter QPainter对象
     * @param rect 渲染区域
     */
    void renderOverlay(QPainter* painter, const QRectF& rect);

    /**
     * @brief 生成性能报告文本
     * @return 性能报告文本
     */
    QString getPerformanceReport() const;

    /**
     * @brief 重置性能数据
     */
    void resetData();

    /**
     * @brief 设置可见的指标类型
     * @param metrics 要显示的指标类型列表
     */
    void setVisibleMetrics(const QVector<MetricType>& metrics);
    
    /**
     * @brief 重置所有测量数据
     */
    void resetMeasurements();
    
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
     * @brief 设置图表显示模式
     * @param mode 显示模式枚举值
     */
    void setChartDisplayMode(ChartDisplayMode mode);
    
    /**
     * @brief 获取图表显示模式
     * @return 当前模式
     */
    ChartDisplayMode getChartDisplayMode() const;
    
    /**
     * @brief 设置覆盖层位置
     * @param position 位置枚举值
     */
    void setOverlayPosition(OverlayPosition position);
    
    /**
     * @brief 获取覆盖层位置
     * @return 当前位置
     */
    OverlayPosition getOverlayPosition() const;
    
    /**
     * @brief 设置覆盖层透明度
     * @param opacity 透明度值(0.0-1.0)
     */
    void setOverlayOpacity(double opacity);
    
    /**
     * @brief 获取覆盖层透明度
     * @return 当前透明度
     */
    double getOverlayOpacity() const;
    
    /**
     * @brief 设置覆盖层大小
     * @param width 宽度
     * @param height 高度
     */
    void setOverlaySize(int width, int height);
    
    /**
     * @brief 计算覆盖层在指定视口中的位置
     * @param viewportRect 视口矩形
     * @return 覆盖层的位置
     */
    QRectF calculateOverlayRect(const QRectF& viewportRect) const;

    /**
     * @brief 注册自定义数据回调函数
     * @param name 回调名称
     * @param callback 回调函数，接收指标映射并填充数据
     */
    void registerMetricCallback(const QString& name, std::function<void(QMap<QString, QVariant>&)> callback);

signals:
    /**
     * @brief 状态变更信号
     * @param enabled 新状态
     */
    void enabledChanged(bool enabled);

    /**
     * @brief 覆盖层状态变更信号
     * @param enabled 新状态
     */
    void overlayEnabledChanged(bool enabled);

    /**
     * @brief 数据更新信号
     */
    void dataUpdated();

    /**
     * @brief 向工作线程发送事件的信号
     * @param event 性能事件
     */
    void sendEvent(const PerformanceEvent& event);

private:
    // 单例模式实现
    PerformanceMonitor(QObject* parent = nullptr);
    ~PerformanceMonitor();
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    // 实例指针，用于isInstanceCreated方法
    static PerformanceMonitor* s_instancePtr;

    // 获取指标的可读名称
    QString getMetricName(MetricType type) const;
    
    // 获取指标的颜色
    QColor getMetricColor(MetricType type) const;

    // 渲染性能图表
    void renderPerformanceGraph(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data);

    // 渲染文本信息
    void renderTextInfo(QPainter* painter, const QRectF& rect);
    
    // 将事件添加到队列
    void enqueueEvent(const PerformanceEvent& event);
    
    // 检查是否需要处理事件队列
    void checkQueue();
    
    // 立即处理队列中的所有事件
    void flushEvents();
    
    // 系统资源监控相关
    void monitorCpuUsage();
    void monitorMemoryUsage();
    void monitorThreadCount();
    void initializeResourceMonitoring();
    double calculateAverage(const QList<qint64>& measurements) const;

    // 覆盖层配置
    ChartDisplayMode m_chartMode = LineChart;
    OverlayPosition m_overlayPosition = TopRight;
    double m_overlayOpacity = 0.85;
    int m_overlayWidth = 350;
    int m_overlayHeight = 300;
    
    // 根据显示模式绘制图表
    void renderChartByMode(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data);
    void renderLineChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal);
    void renderBarChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal);
    void renderAreaChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal);
    void renderDotChart(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data, qint64 maxVal);
    
    // 辅助绘图方法
    void drawChartGrid(QPainter* painter, const QRectF& rect, qint64 maxVal);
    void drawChartLegend(QPainter* painter, const QRectF& rect, const QMap<MetricType, QList<qint64>>& data);

private:
    // 状态缓存（避免频繁的线程同步）
    std::atomic<bool> m_enabled{false};
    std::atomic<bool> m_overlayEnabled{false};
    
    // 事件队列和同步
    QMutex m_queueMutex;
    QQueue<PerformanceEvent> m_eventQueue;
    QWaitCondition m_queueCondition;
    
    // 工作线程和对象
    QThread* m_workerThread;
    PerformanceWorker* m_worker;
    
    // 处理定时器
    QTimer* m_processTimer;
    int m_processingInterval = 50; // ms

    // 图形数据大小设置
    float m_graphHeight = 120.0f;
    int m_graphUpdateInterval = 500; // ms
    
    // 显示选项
    QSet<MetricType> m_visibleMetrics;
    QFont m_overlayFont;
    
    // 渲染缓存
    mutable QMutex m_renderLock;
    mutable QMap<MetricType, QList<qint64>> m_renderData;
    mutable QDateTime m_lastDataSync;
    
    // 自定义指标回调
    QMap<QString, std::function<void(QMap<QString, QVariant>&)>> m_metricCallbacks;
};

// 轻量级性能监控宏定义
#define PERF_START(category) PerformanceMonitor::instance().startMeasure(PerformanceMonitor::category)
#define PERF_END(category) PerformanceMonitor::instance().endMeasure(PerformanceMonitor::category)
#define PERF_SCOPE(category) PerformanceMonitor::ScopedTimer __perf_guard##__LINE__(PerformanceMonitor::category)
#define PERF_FRAME_COMPLETED() PerformanceMonitor::instance().frameCompleted()
#define PERF_EVENT(name, value) PerformanceMonitor::instance().recordEvent(name, value)

#endif // PERFORMANCE_MONITOR_H 