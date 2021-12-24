cmake_minimum_required(VERSION 3.0.0)
project(pa7 VERSION 0.1.0)

add_executable(pa7 main.c args.c bank_robbery.c C.c ipc.c K.c banking.c lamport.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")
set(CMAKE_C_STANDARD 99)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
