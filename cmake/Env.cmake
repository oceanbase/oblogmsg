ob_define(DEVTOOLS_DIR "${CMAKE_SOURCE_DIR}/deps/3rd/usr/local/oceanbase/devtools")
ob_define(DEP_DIR "${CMAKE_SOURCE_DIR}/deps/3rd/usr/local/oceanbase/deps/devel")

set(CMAKE_C_COMPILER "${DEVTOOLS_DIR}/bin/gcc")
set(CMAKE_CXX_COMPILER "${DEVTOOLS_DIR}/bin/g++")
