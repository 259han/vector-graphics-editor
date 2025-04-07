#!/bin/bash
set -euo pipefail

# 定义项目路径
PROJECT_PATH="/f/program/claudegraph"
BUILD_DIR="$PROJECT_PATH/build"

# 进入项目根目录
cd "$PROJECT_PATH"

# 清理旧构建目录（彻底解决残留文件问题）
echo "正在清理旧构建文件..."
rm -rf "$BUILD_DIR"

# 创建全新构建目录
echo "创建构建目录..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 生成构建系统文件
echo "生成CMake项目..."
cmake .. -G "MinGW Makefiles"

# 执行完整构建（添加详细输出便于调试）
echo "开始编译项目..."
cmake --build
echo "构建成功完成！"