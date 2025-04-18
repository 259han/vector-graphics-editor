# 向量图形编辑器 API 文档

**版本**：1.0.0

---

## 目录

1. [核心层 (Core)]
   - [Graphic 类](#graphic-类)
   - [GraphicItem 类](#graphicitem-类)
   - [SelectionManager 类](#selectionmanager-类)
   - [GraphicsItemFactory 类](#graphicsitemfactory-类)
2. [命令层 (Command)](#命令层-command)
   - [Command 接口](#command-接口)
   - [CommandManager 类](#commandmanager-类)
   - [具体命令类](#具体命令类)
3. [状态层 (State)](#状态层-state)
   - [EditorState 类](#editorstate-类)
   - [DrawState 类](#drawstate-类)
   - [EditState 类](#editstate-类)
   - [FillState 类](#fillstate-类)
4. [UI 层 (UI)](#ui-层-ui)
   - [DrawArea 类](#drawarea-类)
   - [ImageResizer 类](#imageresizer-类)
5. [工具层 (Utils)](#工具层-utils)
   - [PerformanceMonitor 类](#performancemonitor-类)
   - [Logger 类](#logger-类)
   - [FileFormatManager 类](#fileformatmanager-类)
   - [GraphicsUtils 类](#graphicsutils-类)

---

## 核心层 (Core)

### Graphic 类

```cpp
class Graphic {
public:
  void draw(QPainter& painter);
  void move(const QPointF& offset);
  void rotate(double angle);
  void scale(double factor);
  void mirror(bool horizontal);
  QPointF getCenter() const;
  GraphicType getType() const;
  QRectF getBoundingBox() const;
  Graphic* clone() const;
  bool contains(const QPointF& point) const;
};
```

- **draw(QPainter& painter)**

  - 描述：在激活的绘图区域中绘制图形；空画笔至少绘制轮廓。
  - 参数：`painter`：有效的 `QPainter` 实例。

- **move(const QPointF& offset)**

  - 描述：按指定偏移量平移图形；`(0,0)` 时跳过。
  - 参数：`offset`：X/Y 方向偏移。

- **rotate(double angle)**

  - 描述：围绕中心顺时针旋转 `angle` 度；`0°` 时无操作。
  - 参数：`angle`：旋转角度（度）。

- **scale(double factor)**

  - 描述：以中心为基准缩放；`factor=1.0` 时跳过。
  - 参数：`factor`：缩放系数。

- **mirror(bool horizontal)**

  - 描述：水平或垂直镜像。
  - 参数：`horizontal`：`true` 为水平，`false` 为垂直。

- **getCenter() / getType() / getBoundingBox() / clone() / contains()**

  - 描述：获取中心点、类型、包围盒、深拷贝及点内检测。

> **异常处理**：
>
> - 参数校验失败时记录警告日志。
> - 空画笔时仅绘制轮廓。

### GraphicItem 类

```cpp
class GraphicItem {
public:
  QRectF getHandleRect(ControlHandle handle) const;
  void moveHandleTo(ControlHandle handle, const QPointF& pos);
  QCursor getCursorForHandle(ControlHandle handle) const;
  void updateControlPoints();
  bool isHandleVisible(ControlHandle handle) const;
  void setHandlesVisible(bool visible);
  int handleCount() const;
};
```

- **控制点接口**：

  - `getHandleRect`：返回指定控制点区域。
  - `moveHandleTo`：移动控制点并自动调整图形。
  - `getCursorForHandle`：获取对应光标形状。

- **交互接口**：

  - `updateControlPoints`：变换后更新控制点位置。
  - `isHandleVisible` / `setHandlesVisible`：控制点可见性。
  - `handleCount`：当前控制点数量。

### SelectionManager 类

```cpp
class SelectionManager {
public:
  void startSelection(const QPointF& startPoint, SelectionMode mode);
  void finishSelection();
  void setSelectionFilter(const SelectionFilter& filter);
};
```

- **startSelection**：

  - 参数：`startPoint`：起始点；`mode`：单选/多选/区域。
  - 异常：无效 `mode` 时默认 `SingleSelection`。

- **finishSelection**：

  - 描述：完成操作并发送 `selectionFinished` 信号。

- **setSelectionFilter**：

  - 描述：设置可选项过滤函数。

### GraphicsItemFactory 类

```cpp
class GraphicsItemFactory {
public:
  QGraphicsItem* createItem(Graphic::GraphicType type, const QPointF& position);
  QGraphicsItem* createCustomItem(Graphic::GraphicType type, const std::vector<QPointF>& points);
};
```

- **createItem**：

  - 参数：`type`：图形类型；`position`：创建位置。
  - 返回：新 `QGraphicsItem` 或 `nullptr`。

- **createCustomItem**：

  - 参数：`type`：图形类型；`points`：顶点集。
  - 返回：基于点集的自定义图形。

> **异常处理**：无效参数时记录错误日志，返回 `nullptr`。

## 命令层 (Command)

### Command 接口

```cpp
class Command {
public:
  virtual void execute() = 0;
  virtual void undo() = 0;
  virtual QString getDescription() const = 0;
  virtual CommandType getType() const = 0;
};
```

- **execute / undo**：执行与撤销，必须对称。
- **getDescription**：用于界面显示与日志。
- **getType**：命令类型标识。

### CommandManager 类

```cpp
class CommandManager {
public:
  void executeCommand(Command* command);
  void undo();
  void beginCommandGroup();
  void endCommandGroup();
};
```

- **executeCommand**：

  - 功能：执行并入撤销栈，清空重做栈。
  - 线程安全：使用 `QReadWriteLock`。
  - 信号：`commandExecuted`。

- **undo**：

  - 功能：撤销最近命令，移至重做栈。
  - 前置：`canUndo() == true`。
  - 信号：`commandUndone`。

- **命令分组**：使用 `beginCommandGroup` / `endCommandGroup`。

### 具体命令类

- **CreateGraphicCommand**：创建图形并添加场景；撤销时移除。
- **TransformCommand**：变换基类；派生 `MoveCommand`、`RotateCommand`、`ScaleCommand`。
- **SelectionCommand**：管理选择状态；撤销恢复。

## 状态层 (State)

### EditorState 类

```cpp
class EditorState {
public:
  virtual void onEnterState(DrawArea* drawArea) = 0;
  virtual void mousePressEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
  virtual void mouseMoveEvent(DrawArea* drawArea, QMouseEvent* event) = 0;
protected:
  void updateStatusMessage(DrawArea* drawArea, const QString& message);
  QPointF getSnapToGridPoint(DrawArea* drawArea, const QPointF& scenePos);
};
```

- **onEnterState**：状态切换时初始化 UI。
- **mousePress/MoveEvent**：处理鼠标事件，控制重绘与性能。

### DrawState 类

- **职责**：图形绘制。
- **方法**：
  - `setGraphicType(type)`：设置绘制类型。
  - 鼠标按下/移动/释放：从预览到命令执行。

### EditState 类

- **职责**：编辑与调整。
- **方法**：
  - 处理选择、拖动、控制点调整。
  - 支持 Shift 多选。

### FillState 类

- **职责**：填充颜色。
- **方法**：
  - `setFillColor(color)`。
  - 点击时执行 `FillCommand`。

## UI 层 (UI)

### DrawArea 类

```cpp
class DrawArea : public QGraphicsView {
public:
  void setDrawState(Graphic::GraphicType type);
  void setEditState();
  void setFillState(const QColor& color);
  void enableGrid(bool enable);
  bool saveToCustomFormat(const QString& filePath);
  void copySelectedItems();
  void pasteItems();
  void copyToSystemClipboard();
};
```

- **状态切换**：绘制/编辑/填充。
- **网格显示**：`enableGrid`。
- **保存/剪贴板**：自定义格式 & 系统剪贴板。

### ImageResizer 类

```cpp
class ImageResizer {
public:
  void setImage(QGraphicsPixmapItem* item);
  bool handleMousePress(const QPointF& pos, Qt::MouseButtons buttons);
  ControlHandle selectHandleUnderPoint(const QPointF& pos);
  void startResize(Handle handle, const QPointF& pos);
  void startRotate(const QPointF& pos);
  void updateResize(const QPointF& pos);
};
```

- **功能**：图像缩放、旋转与控制点操作。

## 工具层 (Utils)

### PerformanceMonitor 类

```cpp
class PerformanceMonitor {
public:
  static PerformanceMonitor& instance();
  void setEnabled(bool enabled);
  void startMeasure(MetricType type, const QString& customName = "");
  void endMeasure(MetricType type);
  QString getPerformanceReport() const;
};
```

- **用途**：帧率、延迟等性能指标测量。
- **线程安全**：内部事件队列。

### Logger 类

```cpp
class Logger {
public:
  static Logger& instance();
  void log(LogLevel level, const QString& message, const QString& file, int line);
  void setLogLevel(LogLevel level);
  void setLogFile(const QString& filePath);
};
```

- **日志宏**：`LOG_DEBUG()` 等。
- **配置**：级别与输出文件。

### FileFormatManager 类

```cpp
class FileFormatManager {
public:
  bool saveToCustomFormat(QGraphicsScene* scene, const QString& filePath);
  bool loadFromCustomFormat(QGraphicsScene* scene, const QString& filePath);
  bool exportToSVG(QGraphicsScene* scene, const QString& filePath, const QSize& size);
};
```

- **格式**：`.cvg`（魔数 `CVGF`、版本、标志位）。
- **SVG 导出**：保留矢量特性。

### GraphicsUtils 类

```cpp
namespace GraphicsUtils {
  QPointF snapToGrid(const QPointF& point, int gridSize);
  QRectF calculateBoundingRect(const std::vector<QPointF>& points);
  QPointF rotatePoint(const QPointF& point, const QPointF& center, double angle);
  QPointF calculateBezierPoint(const std::vector<QPointF>& controlPoints, double t);
  double distancePointToLine(const QPointF& point, const QPointF& start, const QPointF& end);
  double angleBetweenVectors(const QPointF& v1, const QPointF& v2);
}
```

- **算法工具**：对齐网格、包围盒、旋转、贝塞尔计算等。

---

*文档地址：**`/api/api_documentation.md`*

