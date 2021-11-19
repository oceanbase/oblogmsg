set(SRC_LIST LogMsgBuf.cpp
             LogMsgFactory.cpp
             LogRecord.cpp
             MetaInfo.cpp
             MsgType.cpp
             MsgVarArea.cpp
             Crc32.cpp
)

add_library(oblogmsg_shared SHARED ${SRC_LIST})
add_library(oblogmsg_static STATIC ${SRC_LIST})

foreach(T IN ITEMS oblogmsg_shared oblogmsg_static)
  target_compile_features(${T} PUBLIC cxx_std_11)
  target_compile_options(${T} PRIVATE -Wno-invalid-offsetof)

  set_target_properties(${T} PROPERTIES OUTPUT_NAME oblogmsg
                                        CXX_STANDARD 11
                                        CXX_EXTENSIONS OFF
                                        POSITION_INDEPENDENT_CODE ON)
  target_include_directories(${T} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
endforeach()
target_compile_definitions(oblogmsg_static PRIVATE $<$<BOOL:${HIDDEN_DMB_SYMBOLS}>:_HIDDEN_DMB_SYMBOLS>)
# ensure that the build results can be run on systems with lower libstdc++ version than the build system
target_link_libraries(oblogmsg_shared PRIVATE -static-libstdc++)

# with namespace oceanbase
add_library(oceanbase::oblogmsg_shared ALIAS oblogmsg_shared)
add_library(oceanbase::oblogmsg_static ALIAS oblogmsg_static)
