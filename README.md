<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>矢量图形编辑器 | Vector Graphics Editor</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            max-width: 900px;
            margin: 0 auto;
            padding: 20px;
        }
        .lang-switch {
            position: fixed;
            top: 20px;
            right: 20px;
            z-index: 100;
        }
        .lang-switch button {
            padding: 8px 12px;
            margin-left: 5px;
            background-color: #f0f0f0;
            border: 1px solid #ddd;
            border-radius: 4px;
            cursor: pointer;
            transition: background-color 0.3s;
        }
        .lang-switch button:hover {
            background-color: #e0e0e0;
        }
        .lang-content {
            display: none;
        }
        .lang-content.active {
            display: block;
        }
        pre {
            background-color: #f6f8fa;
            border-radius: 4px;
            padding: 16px;
            overflow: auto;
        }
        code {
            font-family: 'Consolas', 'Courier New', monospace;
        }
        h1, h2 {
            border-bottom: 1px solid #eaecef;
            padding-bottom: 0.3em;
        }
    </style>
</head>
<body>
    <div class="lang-switch">
        <button onclick="switchLang('zh')">中文</button>
        <button onclick="switchLang('en')">English</button>
    </div>

    <div id="zh" class="lang-content active">
        <h1>矢量图形编辑器</h1>

        <h2>项目简介</h2>
        <p>这是一个基于Qt6开发的跨平台矢量图形编辑器，提供直观的用户界面和一定的图形编辑功能。采用命令模式和状态管理，支持高效的图形操作和撤销/重做功能。</p>

        <h2>技术栈</h2>
        <ul>
            <li>编程语言：C++17</li>
            <li>GUI框架：Qt6 (Widgets, Svg)</li>
            <li>构建系统：CMake 3.16+</li>
            <li>编译器支持：MSVC、MinGW、GCC、Clang</li>
        </ul>

        <h2>项目结构</h2>
        <pre><code>.
├── src/                # 源代码目录
│   ├── command/       # 命令模式实现（撤销/重做）
│   ├── state/        # 状态管理（工具状态）
│   ├── ui/           # 用户界面组件
│   ├── core/         # 核心功能（图形元素、绘制逻辑）
│   ├── utils/        # 工具类（日志、性能监控）
│   └── main.cpp      # 程序入口
├── build/            # 构建输出目录
├── .vscode/         # VS Code配置
├── logs/            # 日志输出目录
├── CMakeLists.txt    # CMake主配置文件
├── CMakePresets.json # CMake预设配置
├── bd.sh            # 构建脚本
└── .gitignore       # Git忽略文件</code></pre>

        <h2>构建指南</h2>

        <h3>环境需求</h3>
        <ul>
            <li>Qt6开发环境</li>
            <li>CMake 3.16或更高版本</li>
            <li>C++17兼容的编译器</li>
        </ul>

        <h3>使用CMake手动构建</h3>
        <pre><code># 创建并进入构建目录
mkdir build && cd build

# 配置项目（选择适合的生成器，例如MinGW或MSVC）
cmake ..

# 编译项目
cmake --build . --config Release</code></pre>

        <h3>使用构建脚本（Windows + MinGW）</h3>
        <pre><code># 普通构建
./bd.sh

# 清理并重新构建
./bd.sh clean

# 调试模式构建
./bd.sh debug

# 构建后运行
./bd.sh run

# 组合使用
./bd.sh clean debug run</code></pre>

        <h2>功能特性</h2>
        <ul>
            <li>基本形状绘制：矩形、椭圆、多边形、线条等</li>
            <li>SVG格式导入/导出</li>
            <li>图层管理和对象变换</li>
            <li>命令模式实现的操作历史（撤销/重做）</li>
            <li>性能监控系统（Windows平台）</li>
            <li>全面的日志系统</li>
        </ul>

        <h2>开发环境</h2>
        <ul>
            <li>跨平台支持：Windows、Linux和macOS</li>
            <li>推荐IDE：Visual Studio Code（提供预配置的调试和任务设置）</li>
            <li>UTF-8编码支持（MSVC编译器使用/utf-8选项）</li>
            <li>支持MinGW和MSVC编译环境</li>
        </ul>
    </div>

    <div id="en" class="lang-content">
        <h1>Vector Graphics Editor</h1>

        <h2>Project Overview</h2>
        <p>A cross-platform vector graphics editor developed with Qt6, featuring an intuitive user interface and powerful graphics editing capabilities. Implements command pattern and state management for efficient graphic operations and undo/redo functionality.</p>

        <h2>Tech Stack</h2>
        <ul>
            <li>Programming Language: C++17</li>
            <li>GUI Framework: Qt6 (Widgets, Svg)</li>
            <li>Build System: CMake 3.16+</li>
            <li>Compiler Support: MSVC, MinGW, GCC, Clang</li>
        </ul>

        <h2>Project Structure</h2>
        <pre><code>.
├── src/                # Source code directory
│   ├── command/       # Command pattern implementation (undo/redo)
│   ├── state/        # State management (tool states)
│   ├── ui/           # User interface components
│   ├── core/         # Core functionality (graphic elements, drawing logic)
│   ├── utils/        # Utility classes (logging, performance monitoring)
│   └── main.cpp      # Program entry point
├── build/            # Build output directory
├── .vscode/         # VS Code configuration
├── logs/            # Log output directory
├── CMakeLists.txt    # CMake main configuration
├── CMakePresets.json # CMake presets configuration
├── bd.sh            # Build script
└── .gitignore       # Git ignore file</code></pre>

        <h2>Build Guide</h2>

        <h3>Requirements</h3>
        <ul>
            <li>Qt6 development environment</li>
            <li>CMake 3.16 or higher</li>
            <li>C++17 compatible compiler</li>
        </ul>

        <h3>Manual Build with CMake</h3>
        <pre><code># Create and enter build directory
mkdir build && cd build

# Configure project (choose appropriate generator like MinGW or MSVC)
cmake ..

# Build project
cmake --build . --config Release</code></pre>

        <h3>Using Build Script (Windows + MinGW)</h3>
        <pre><code># Standard build
./bd.sh

# Clean and rebuild
./bd.sh clean

# Debug mode build
./bd.sh debug

# Build and run
./bd.sh run

# Combined options
./bd.sh clean debug run</code></pre>

        <h2>Features</h2>
        <ul>
            <li>Basic shape drawing: rectangles, ellipses, polygons, lines, etc.</li>
            <li>SVG format import/export</li>
            <li>Layer management and object transformations</li>
            <li>Operation history with command pattern (undo/redo)</li>
            <li>Performance monitoring system (Windows platform)</li>
            <li>Comprehensive logging system</li>
        </ul>

        <h2>Development Environment</h2>
        <ul>
            <li>Cross-platform support: Windows, Linux, and macOS</li>
            <li>Recommended IDE: Visual Studio Code (with preconfigured debugging and task settings)</li>
            <li>UTF-8 encoding support (MSVC compiler with /utf-8 option)</li>
            <li>Support for both MinGW and MSVC build environments</li>
        </ul>
    </div>

    <script>
        function switchLang(lang) {
            document.querySelectorAll('.lang-content').forEach(content => {
                content.classList.remove('active');
            });
            document.getElementById(lang).classList.add('active');
        }
    </script>
</body>
</html> 