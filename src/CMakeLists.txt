cmake_minimum_required(VERSION 3.10)
#
project(BootStrapers)
#


set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES scheduler.c process_generator.c pcb.c process.c clk.c)

add_executable(my_program ${SOURCE_FILES})



#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED True)
#
#add_executable(MyExecutable process.c scheduler.c)
#
#include_directories(${PROJECT_SOURCE_DIR}/src)
#
#target_link_libraries(MyExecutable)
