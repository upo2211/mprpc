# 设置最低版本号
cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(mprpc)

# 设置gdb调试
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 设置库文件的输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(/usr/local/include)
include_directories(${PROJECT_SOURCE_DIR}/example)

# 设置库文件的搜索路径 exmaple中的callee和caller都要用到框架生成的库文件
link_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(/usr/local/lib)

add_subdirectory(src)
add_subdirectory(example)