ob_define(DEVTOOLS_DIR "${CMAKE_SOURCE_DIR}/deps/3rd/usr/local/oceanbase/devtools")
ob_define(DEP_DIR "${CMAKE_SOURCE_DIR}/deps/3rd/usr/local/oceanbase/deps/devel")

set(CMAKE_C_COMPILER "${DEVTOOLS_DIR}/bin/clang")
set(CMAKE_CXX_COMPILER "${DEVTOOLS_DIR}/bin/clang++")

ob_define(GCC9 "${DEVTOOLS_DIR}")

set(CMAKE_CXX_FLAGS "--gcc-toolchain=${GCC9}")
set(CMAKE_C_FLAGS "--gcc-toolchain=${GCC9}")
