add_executable(lr_unittest lrTest.cpp)
target_link_libraries(lr_unittest PRIVATE oblogmsg_shared gtest)

add_executable(varArea_unittest varAreaTest.cpp)
target_include_directories(varArea_unittest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(varArea_unittest PRIVATE oblogmsg_shared gtest)

add_executable(md_unittest mdTest.cpp)
target_link_libraries(md_unittest PRIVATE oblogmsg_shared gtest)

add_executable(fa_unittest faTest.cpp)
target_link_libraries(fa_unittest PRIVATE oblogmsg_shared gtest)

add_executable(memoryleak_unittest memoryleakTest.cpp)
target_link_libraries(memoryleak_unittest PRIVATE oblogmsg_shared gtest)

enable_testing()

add_test(lr_unittest lr_unittest)
add_test(md_unittest md_unittest)
add_test(fa_unittest fa_unittest)
add_test(varArea_unittest varArea_unittest)
add_test(memoryleak_unittest memoryleak_unittest)
