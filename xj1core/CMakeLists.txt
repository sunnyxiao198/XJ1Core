# XJ1Core ESP32-S3 项目
cmake_minimum_required(VERSION 3.16)

# 设置项目名称
set(PROJECT_NAME "xj1core")

# 包含ESP-IDF构建系统
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 定义项目
project(${PROJECT_NAME})
