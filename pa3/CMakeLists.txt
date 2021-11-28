cmake_minimum_required(VERSION 3.0.0)
project(pa3 VERSION 0.1.0)

add_executable(pa3 main.c args.c bank_robbery.c C.c ipc.c K.c banking.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -L ${PROJECT_SOURCE_DIR}/lib64 -lruntime")
set(CMAKE_C_STANDARD 99)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
