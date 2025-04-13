#!/bin/bashset -euo pipefail

# 定义项目路径
PROJECT_PATH="/f/program/claudegraph"
BUILD_DIR="$PROJECT_PATH/build"

# 进入项目根目录
cd "$PROJECT_PATH"

# 处理命令行参数
CLEAN_BUILD=0
DEBUG_MODE=0
RUN_AFTER_BUILD=0

if [ $# -gt 0 ]; then
    for arg in "$@"; do
        if [ "$arg" == "clean" ]; then
            CLEAN_BUILD=1
            echo "将执行清理构建..."
        elif [ "$arg" == "debug" ]; then
            DEBUG_MODE=1
            echo "将使用调试模式构建..."
        elif [ "$arg" == "run" ]; then
            RUN_AFTER_BUILD=1
            echo "构建后将运行程序..."
        fi
    done
fi

# 只有在指定clean参数或build目录不存在时才清理
if [ $CLEAN_BUILD -eq 1 ] || [ ! -d "$BUILD_DIR" ]; then
    echo "正在清理旧构建文件..."
    rm -rf "$BUILD_DIR"
    
    # 创建全新构建目录
    echo "创建构建目录..."
    mkdir -p "$BUILD_DIR"
    
    # 设置需要重新生成CMake配置
    NEED_CMAKE=1
else
    echo "使用现有构建目录..."
    # 如果CMakeCache.txt不存在，需要重新生成CMake配置
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        NEED_CMAKE=1
    else
        NEED_CMAKE=0
    fi
fi

# 进入构建目录
cd "$BUILD_DIR"

# 只在需要时生成CMake文件
if [ ${NEED_CMAKE:-1} -eq 1 ]; then
    echo "生成CMake项目..."
    if [ $DEBUG_MODE -eq 1 ]; then
        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
    else
        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
    fi
else
    echo "使用现有CMake配置..."
fi

# 执行构建
echo "开始编译项目..."
mingw32-make -j4
echo "构建成功完成！"

# 如果构建成功，显示可执行文件路径
if [ -f "$BUILD_DIR/vector-graphics-editor.exe" ]; then
    echo "可执行文件位置: $BUILD_DIR/vector-graphics-editor.exe"
    
    # 只有在指定了run参数时才运行程序
    if [ $RUN_AFTER_BUILD -eq 1 ]; then
        echo "正在运行程序..."
        "$BUILD_DIR/vector-graphics-editor.exe"
    fi
fi