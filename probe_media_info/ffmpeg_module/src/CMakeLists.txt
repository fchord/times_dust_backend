# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project (ffmpeg_probe)

# 设置编译选项
set(CMAKE_CXX_FLAGS "-std=c++14 -fPIC -shared -g -O0 -pg")

# 设置安装路径
# set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/../lib/)

# 添加动态库搜索目录
link_directories(${PROJECT_SOURCE_DIR}/../lib)
link_directories(${PROJECT_SOURCE_DIR}/../lib/ffmpeg/n4.4)
link_directories(${PROJECT_SOURCE_DIR}/../lib/x264)
link_directories(${PROJECT_SOURCE_DIR}/../lib/fdk_aac)
link_directories(${PROJECT_SOURCE_DIR}/../lib/spdlog/v1.7.0)

# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/../inc/ DIR_INCS)
include_directories(${PROJECT_SOURCE_DIR}/../inc/ffmpeg/n4.4 DIR_INCS_FFMPEG)
include_directories(${PROJECT_SOURCE_DIR}/../inc/boost_1_80_0_depends/ DIR_INCS_BOOST)
include_directories(${PROJECT_SOURCE_DIR}/../inc/spdlog/v1.7.0/ DIR_INCS_SPDLOG)


# 查找当前目录下的所有源文件 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)

# 指定生成目标
# add_executable(${PROJECT_NAME} ${DIR_SRCS})
add_library(ffmpeg_probe SHARED ${DIR_SRCS} )

# 设置安装路径
install(TARGETS ffmpeg_probe DESTINATION ${PROJECT_SOURCE_DIR}/../lib/)


# 添加动态库
target_link_libraries(${PROJECT_NAME} libavformat.so)
target_link_libraries(${PROJECT_NAME} libavcodec.so)
target_link_libraries(${PROJECT_NAME} libavfilter.so)
target_link_libraries(${PROJECT_NAME} libavutil.so)
target_link_libraries(${PROJECT_NAME} libx264.so)
target_link_libraries(${PROJECT_NAME} libfdk-aac.so)
target_link_libraries(${PROJECT_NAME} libpthread.so)

target_link_libraries(${PROJECT_NAME} libspdlog.a)
