set(SRC_LIST
    MsgType.cpp
    LogMsgBuf.cpp
    MsgVarArea.cpp 
    LogRecord.cpp
    MetaInfo.cpp 
    LogMsgFactory.cpp
)
add_library(oblogmsg SHARED ${SRC_LIST})
add_library(oblogmsg_static STATIC ${SRC_LIST})
target_link_libraries(oblogmsg PUBLIC oblogmsg_base)
target_link_libraries(oblogmsg_static PUBLIC oblogmsg_base)
set_target_properties(oblogmsg_static PROPERTIES OUTPUT_NAME "oblogmsg")
