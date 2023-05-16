cmake_minimum_required(VERSION 3.20.0)

if(BUILD_AS_MAIN_PROJECT)
  message(STATUS "oblogmsg build as main project")
  include(cmake/Utils.cmake)
  include(cmake/Env.cmake)
else()
  message(STATUS "oblogmsg build as sub project")
endif()

project(oblogmsg VERSION 3.2.1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ..." FORCE)
endif()

add_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:_GLIBCXX_USE_CXX11_ABI=0>)

add_compile_options($<$<COMPILE_LANGUAGE:CXX,C>:-fstack-protector-strong>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX,C>:-fPIC>)

add_link_options($<$<COMPILE_LANGUAGE:CXX,C>:-Wl,-z,relro,-z,now,z,noexecstack>)
#add_link_options($<$<COMPILE_LANGUAGE:CXX,C>:-pie>)

add_subdirectory(src)

# Check if oblogmsg is being used directly or via add_subdirectory
set(OBLOGMSG_MASTER_PROJECT OFF)
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(OBLOGMSG_MASTER_PROJECT ON)
endif()

if(OBLOGMSG_MASTER_PROJECT)
  message(STATUS "git submodule update --init third_party/googletest")
  execute_process(COMMAND git submodule update --init third_party/googletest
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  COMMAND_ERROR_IS_FATAL ANY)
  set(BUILD_GMOCK OFF CACHE INTERNAL "Builds the googlemock subproject")
  set(INSTALL_GTEST OFF CACHE INTERNAL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)")
  add_subdirectory(third_party/googletest)
  add_subdirectory(unittest)

  add_subdirectory(benchmark)
endif()

# If oblogmsg is used via add_subdirectory, by default we don't install it.
option(INSTALL_OBLOGMSG "Whether or not install oblogmsg" ${OBLOGMSG_MASTER_PROJECT})

if(INSTALL_OBLOGMSG)
  ################# INSTALL ##########################################################################
  include(GNUInstallDirs)

  install(DIRECTORY include/
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  )

  install(TARGETS oblogmsg_shared oblogmsg_static
          EXPORT oblogmsg
          LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
          INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  )

  install(EXPORT oblogmsg
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/oblogmsg
          FILE oblogmsg.cmake
          NAMESPACE oceanbase::
  )

  ## install oblogmsgConfig.cmake and oblogmsgConfigVersion.cmake
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/oblogmsgConfig.cmake "include(\${CMAKE_CURRENT_LIST_DIR}/oblogmsg.cmake)\n")
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/oblogmsgConfigVersion.cmake
                                   COMPATIBILITY SameMajorVersion
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/oblogmsgConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/oblogmsgConfigVersion.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/oblogmsg
  )

  ################# PACKAGE ##########################################################################
  set(CPACK_GENERATOR "RPM")
  set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
  set(CPACK_RPM_COMPONENT_INSTALL ON)
  set(CPACK_RPM_RELOCATION_PATHS /usr /home/admin/oceanbase)
  set(CPACK_PACKAGING_INSTALL_PREFIX /home/admin/oceanbase)
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
endif()
