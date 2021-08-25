set(SRC_LIST
    lmTest.cpp
    itoa.cpp
)
add_executable(lmTest ${SRC_LIST})
target_link_libraries(lmTest PUBLIC oblogmsg)
target_compile_options(lmTest PRIVATE -DLMB)
