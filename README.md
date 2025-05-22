# 矢量图形编辑器 | Vector Graphics Editor

[中文](#中文) | [English](#english)

## 中文

### 项目简介

这是一个基于Qt6开发的跨平台矢量图形编辑器，提供直观的用户界面和丰富的图形编辑功能。采用命令模式和状态管理，支持高效的图形操作和撤销/重做功能。

### 最新功能

- **高级裁剪功能**：支持矩形裁剪和自由形状裁剪，可处理复杂形状
- **非规则图形处理**：裁剪后的非规则图形仍可自由移动、旋转和缩放
- **边缘智能检测**：优化的边缘检测算法，使操作更加精准

### 技术栈

- 编程语言：C++17
- GUI框架：Qt6 (Widgets, Svg)
- 构建系统：CMake 3.16+
- 编译器支持：MSVC、MinGW、GCC、Clang

### 项目结构

```
.
├── src/                # 源代码目录
│   ├── command/        # 命令模式实现（撤销/重做）
│   ├── state/          # 状态管理（工具状态）
│   │   └── clip_state.cpp   # 裁剪状态实现
│   ├── ui/             # 用户界面组件
│   ├── core/           # 核心功能（图形元素、绘制逻辑）
│   │   ├── rectangle_graphic_item.cpp  # 矩形图形实现
│   │   └── ellipse_graphic_item.cpp    # 椭圆图形实现
│   ├── utils/          # 工具类（日志、性能监控）
│   │   └── clip_algorithms.cpp         # 裁剪算法实现
│   └── main.cpp        # 程序入口
├── build/              # 构建输出目录
├── .vscode/            # VS Code配置
├── logs/               # 日志输出目录
├── CMakeLists.txt      # CMake主配置文件
├── CMakePresets.json   # CMake预设配置
├── bd.sh               # 构建脚本
└── .gitignore          # Git忽略文件
```

### 构建指南

#### 环境需求
- Qt6开发环境
- CMake 3.16或更高版本
- C++17兼容的编译器

#### 使用CMake手动构建
```bash
# 创建并进入构建目录
mkdir build && cd build

# 配置项目（选择适合的生成器，例如MinGW或MSVC）
cmake ..

# 编译项目
cmake --build . --config Release
```

#### 使用构建脚本（Windows + MinGW）
```bash
# 普通构建
./bd.sh

# 清理并重新构建
./bd.sh clean

# 调试模式构建
./bd.sh debug

# 构建后运行
./bd.sh run

# 组合使用
./bd.sh clean debug run
```

### 功能特性

- **基本形状绘制**：矩形、椭圆、多边形、线条等
- **高级裁剪工具**：
  - 矩形裁剪：使用矩形区域裁剪任意图形
  - 自由形状裁剪：使用自定义路径裁剪图形，支持复杂形状
  - 智能边缘检测：裁剪后图形保持可移动性
- **SVG格式导入/导出**
- **图层管理和对象变换**
- **命令模式实现的操作历史**（撤销/重做）
- **性能监控系统**（Windows平台）
- **全面的日志系统**

### 开发环境

- 跨平台支持：Windows、Linux和macOS
- 推荐IDE：Visual Studio Code（提供预配置的调试和任务设置）
- UTF-8编码支持（MSVC编译器使用/utf-8选项）
- 支持MinGW和MSVC编译环境

---

## English

### Project Overview

A cross-platform vector graphics editor developed with Qt6, featuring an intuitive user interface and powerful graphics editing capabilities. Implements command pattern and state management for efficient graphic operations and undo/redo functionality.

### Latest Features

- **Advanced Clipping**: Support for rectangular and freeform clipping, handling complex shapes
- **Irregular Shape Handling**: Clipped irregular shapes remain fully movable, rotatable, and scalable
- **Smart Edge Detection**: Optimized edge detection algorithm for more precise operations

### Tech Stack

- Programming Language: C++17
- GUI Framework: Qt6 (Widgets, Svg)
- Build System: CMake 3.16+
- Compiler Support: MSVC, MinGW, GCC, Clang

### Project Structure

```
.
├── src/                # Source code directory
│   ├── command/        # Command pattern implementation (undo/redo)
│   ├── state/          # State management (tool states)
│   │   └── clip_state.cpp   # Clipping state implementation
│   ├── ui/             # User interface components
│   ├── core/           # Core functionality (graphic elements, drawing logic)
│   │   ├── rectangle_graphic_item.cpp  # Rectangle graphic implementation
│   │   └── ellipse_graphic_item.cpp    # Ellipse graphic implementation
│   ├── utils/          # Utility classes (logging, performance monitoring)
│   │   └── clip_algorithms.cpp         # Clipping algorithms implementation
│   └── main.cpp        # Program entry point
├── build/              # Build output directory
├── .vscode/            # VS Code configuration
├── logs/               # Log output directory
├── CMakeLists.txt      # CMake main configuration
├── CMakePresets.json   # CMake presets configuration
├── bd.sh               # Build script
└── .gitignore          # Git ignore file
```

### Build Guide

#### Requirements
- Qt6 development environment
- CMake 3.16 or higher
- C++17 compatible compiler

#### Manual Build with CMake
```bash
# Create and enter build directory
mkdir build && cd build

# Configure project (choose appropriate generator like MinGW or MSVC)
cmake ..

# Build project
cmake --build . --config Release
```

#### Using Build Script (Windows + MinGW)
```bash
# Standard build
./bd.sh

# Clean and rebuild
./bd.sh clean

# Debug mode build
./bd.sh debug

# Build and run
./bd.sh run

# Combined options
./bd.sh clean debug run
```

### Features

- **Basic shape drawing**: rectangles, ellipses, polygons, lines, etc.
- **Advanced clipping tools**:
  - Rectangle clipping: Clip any shape with a rectangular region
  - Freeform clipping: Clip shapes with custom paths, supporting complex shapes
  - Smart edge detection: Clipped shapes maintain movability
- **SVG format import/export**
- **Layer management and object transformations**
- **Operation history with command pattern** (undo/redo)
- **Performance monitoring system** (Windows platform)
- **Comprehensive logging system**

### Development Environment

- Cross-platform support: Windows, Linux, and macOS
- Recommended IDE: Visual Studio Code (with preconfigured debugging and task settings)
- UTF-8 encoding support (MSVC compiler with /utf-8 option)
- Support for both MinGW and MSVC build environments 