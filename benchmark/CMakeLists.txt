add_executable(lmTest itoa.cpp lmTest.cpp)
target_compile_definitions(lmTest PRIVATE LMB)
target_include_directories(lmTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lmTest PRIVATE oblogmsg_shared)
