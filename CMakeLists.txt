cmake_minimum_required(VERSION 3.16)
project(vector-graphics-editor VERSION 1.0)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用自动 MOC、RCC 和 UIC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

# 添加源文件 - 排除根目录的 main.cpp，只使用 src/main.cpp
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# 添加可执行文件，使用 WIN32 选项来隐藏控制台窗口
add_executable(${PROJECT_NAME}
    src/main.cpp 
    ${SOURCES}
)

# 设置目标包含目录
target_include_directories(${PROJECT_NAME} PRIVATE include src)

# 链接 Qt6::Widgets 库
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    Qt6::Widgets
    Qt6::Svg
)

# 在Windows平台下链接PDH库（用于CPU性能监控）
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE pdh)
endif()

# 设置编译器选项
if(MSVC)
    # 为 MSVC 添加 /utf-8 和 /Zc:__cplusplus 选项
    target_compile_options(${PROJECT_NAME} PRIVATE "/utf-8" "/Zc:__cplusplus")
else()
    # 为其他编译器添加 -fPIC 选项（如果是 GCC 或 Clang）
    target_compile_options(${PROJECT_NAME} PRIVATE "-fPIC")
endif()