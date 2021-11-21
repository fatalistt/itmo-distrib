cmake_minimum_required(VERSION 3.0.0)
project(lab1 VERSION 0.1.0)

add_executable(lab1 main.c args.c args.h child.c child.h common.h pa1.h parent.c parent.h ipc.c ipc.h)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")
set(CMAKE_C_STANDARD 99)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
