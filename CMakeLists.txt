project(xdebug)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${xdebug_SOURCE_DIR}/cmake/)

remove_definitions( -DKDE_DEFAULT_DEBUG_AREA=24000 )
add_definitions( -DKDE_DEFAULT_DEBUG_AREA=9046 )


enable_testing()
find_package(KDE4 REQUIRED)
find_package(KDevPlatform REQUIRED)

find_path( EXECUTESCRIPT_INCLUDE_DIR iexecutescriptplugin.h
 PATH_SUFFIXES kdevexecutescript )

find_path( EXECUTEBROWSER_INCLUDE_DIR iexecutebrowserplugin.h
 PATH_SUFFIXES kdevexecutebrowser )

include_directories(
    ${KDEVPLATFORM_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EXECUTESCRIPT_INCLUDE_DIR}
    ${EXECUTEBROWSER_INCLUDE_DIR}
)

########### next target ###############

set(kdevxdebugcore_PART_SRCS
    connection.cpp
    debugsession.cpp
    debugjob.cpp
    framestackmodel.cpp
    breakpointcontroller.cpp
    variablecontroller.cpp
    variable.cpp
    stringhelpers.cpp
)

set(kdevxdebug_PART_SRCS
    xdebugplugin.cpp
    launchconfig.cpp
    launchconfigurationpage.cpp
)

kde4_add_plugin(kdevxdebug ${kdevxdebug_PART_SRCS} ${kdevxdebugcore_PART_SRCS})

target_link_libraries(kdevxdebug
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_DEBUGGER_LIBRARIES}
    ${KDEVPLATFORM_OUTPUTVIEW_LIBRARIES}
    ${KDEVPLATFORM_UTIL_LIBRARIES}
)

install(TARGETS kdevxdebug DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(connectiontest_SRCS
    tests/connectiontest.cpp
)

kde4_add_unit_test(connectiontest ${connectiontest_SRCS} ${kdevxdebugcore_PART_SRCS})

target_link_libraries(connectiontest
    ${QT_QTTEST_LIBRARY}
    ${KDEVPLATFORM_INTERFACES_LIBRARIES}
    ${KDEVPLATFORM_TESTSHELL_LIBRARIES}
    ${KDEVPLATFORM_DEBUGGER_LIBRARIES}
    ${KDEVPLATFORM_TESTS_LIBRARIES}
    ${KDEVPLATFORM_OUTPUTVIEW_LIBRARIES}
    ${KDEVPLATFORM_UTIL_LIBRARIES}
)


########### install files ###############

install(FILES kdevxdebug.desktop DESTINATION ${SERVICES_INSTALL_DIR})
