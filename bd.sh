#!/bin/bash
set -e  # 遇到错误时退出

# 自动检测平台和工作目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"

# 检测操作系统
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    IS_WINDOWS=1
    MAKE_CMD="mingw32-make"
    CMAKE_GENERATOR="MinGW Makefiles"
else
    IS_WINDOWS=0
    MAKE_CMD="make"
    CMAKE_GENERATOR="Unix Makefiles"
fi

echo "检测到构建环境:"
echo "- 工作目录: $SCRIPT_DIR"
echo "- 构建目录: $BUILD_DIR"
echo "- 操作系统: ${IS_WINDOWS:-Linux/Unix}"
echo "- 生成器: $CMAKE_GENERATOR"
echo "- 构建工具: $MAKE_CMD"

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
        cmake .. -G "$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE=Debug
    else
        cmake .. -G "$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE=Release
    fi
else
    echo "使用现有CMake配置..."
fi

# 执行构建
echo "开始编译项目..."
$MAKE_CMD -j$(nproc 2>/dev/null || echo 4)
echo "构建成功完成！"

# 如果构建成功，运行程序（如果需要）
if [ $RUN_AFTER_BUILD -eq 1 ]; then
    echo "正在运行程序..."
    
    # 检测可执行文件名
    if [ $IS_WINDOWS -eq 1 ]; then
        EXE_NAME="vector-graphics-editor.exe"
    else
        EXE_NAME="vector-graphics-editor"
    fi
    
    # 检查可执行文件是否存在
    if [ -f "$BUILD_DIR/$EXE_NAME" ]; then
        "$BUILD_DIR/$EXE_NAME"
    else
        echo "错误: 找不到可执行文件 $EXE_NAME"
        exit 1
    fi
fi