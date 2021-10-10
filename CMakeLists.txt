cmake_minimum_required(VERSION 3.14)
project(oblogmsg)

option(TEST "build tests" OFF)
option(BENCHMARK "build benchmarks" OFF)
option(USE_CXX11_ABI "Build with C++11 ABI" OFF)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_INSTALL_PREFIX /home/ds)

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

set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "oblogmsg")
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_RELOCATION_PATHS /usr /home/ds)
set(CPACK_PACKAGING_INSTALL_PREFIX /home/ds)
set(CPACK_PACKAGE_VERSION 3.2)
set(CPACK_RPM_PACKAGE_GROUP "Applications/Databases")
set(CPACK_RPM_PACKAGE_LICENSE "Mulan PubL v2.")
set(CPACK_RPM_DEFAULT_USER "admin")
set(CPACK_RPM_DEFAULT_GROUP "admin")
set(CPACK_RPM_SPEC_MORE_DEFINE
  "%global _missing_build_ids_terminate_build 0
%global _find_debuginfo_opts -g
%define __debug_install_post %{_rpmconfigdir}/find-debuginfo.sh %{?_find_debuginfo_opts} %{_builddir}/%{?buildsubdir};%{nil}
%define debug_package %{nil}")
include(CPack)

add_custom_target(rpm COMMAND +make package)
