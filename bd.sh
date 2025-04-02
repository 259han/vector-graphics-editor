#!/bin/bash

# 定义项目路径
PROJECT_PATH="/f/program/claudegraph"
BUILD_DIR="$PROJECT_PATH/build"

# 进入项目目录
cd "$PROJECT_PATH"

# 检查 build 文件夹是否存在
if [ ! -d "$BUILD_DIR" ]; then
    # 如果不存在，创建 build 文件夹
    mkdir -p "$BUILD_DIR"
fi

# 进入 build 文件夹
cd "$BUILD_DIR"

# 运行 CMake 配置
cmake .. -G "MinGW Makefiles"

# 使用 --clean-first 选项清理旧的构建文件并重新构建
cmake --build . --clean-first