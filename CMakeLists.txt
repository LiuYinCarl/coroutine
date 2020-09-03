cmake_minimum_required(VERSION 3.10)

project(coro)

# set(CMAKE_C_FLAGS "-std=c99")  # CentOS7 下使用 C99 标准会出现  unknown type name ‘stack_t’ 错误
add_executable(coro main.c co.c)