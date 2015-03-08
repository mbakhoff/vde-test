cmake_minimum_required(VERSION 2.8.4)
project(vde_test)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -g")

set(SOURCE_FILES
    src/common.h
    src/common.c
    src/channel.h
    src/channel.c
    src/posix-socket.h
    src/posix-socket.c
    src/vde-io.h
    src/vde-io.c
    src/main.c)

add_executable(vde_test ${SOURCE_FILES})
target_link_libraries(vde_test -lvdeplug)
