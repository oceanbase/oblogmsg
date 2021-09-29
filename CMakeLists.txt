cmake_minimum_required(VERSION 3.14)
project(oblogmsg)

option(TEST "build tests" OFF)
option(BENCHMARK "build benchmarks" OFF)
option(USE_CXX11_ABI "Build with C++11 ABI" OFF)

set(CMAKE_VERBOSE_MAKEFILE OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif ()

if (NOT USE_CXX11_ABI)
    add_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:_GLIBCXX_USE_CXX11_ABI=0>)
endif ()

add_library(oblogmsg_base INTERFACE)
set(INNER_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(GLOBAL PROPERTY OBLOGMSG_INCLUDE_DIRS ${INNER_INCLUDE_DIRS})
target_include_directories(oblogmsg_base INTERFACE ${INNER_INCLUDE_DIRS})
target_compile_features(oblogmsg_base INTERFACE cxx_std_11)

add_subdirectory(src)

if (TEST)
    execute_process(
            COMMAND git submodule init
            COMMAND git submodule update
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    add_subdirectory(third_party/googletest)
    add_subdirectory(unittest)
    enable_testing()
endif ()

if (BENCHMARK)
    add_subdirectory(benchmark)
endif ()

install(TARGETS oblogmsg oblogmsg_static
        LIBRARY DESTINATION lib64
        ARCHIVE DESTINATION lib64
)

install(
        DIRECTORY include/ DESTINATION include/logmessage
)
