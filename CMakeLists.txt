cmake_minimum_required(VERSION 3.0.0)
project(xdebug)

find_package (ECM "5.14.0" REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(GenerateExportHeader)
include(FeatureSummary)

set(QT_MIN_VERSION "5.5.0")
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Test)
set(KF5_DEP_VERSION "5.15.0")
find_package(KF5 ${KF5_DEP_VERSION} REQUIRED COMPONENTS TextEditor I18n ItemModels)

find_package(KDevPlatform 5.1 CONFIG)
set_package_properties(KDevPlatform PROPERTIES
    TYPE REQUIRED
)

# TODO: That plugin should install a CMake config file
find_path(EXECUTEBROWSER_INCLUDE_DIR iexecutebrowserplugin.h PATH_SUFFIXES kdevexecutebrowser)
include_directories(${EXECUTEBROWSER_INCLUDE_DIR})

set(CMAKE_AUTOUIC ON)

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
    launchconfigurationpage.ui
)
kdevplatform_add_plugin(kdevxdebug SOURCES ${kdevxdebug_PART_SRCS} ${kdevxdebugcore_PART_SRCS})
target_link_libraries(kdevxdebug
    KDev::Interfaces
    KDev::Debugger
    KDev::OutputView
    KDev::Shell
    KDev::Util
)

configure_file(kdevxdebug.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdevxdebug.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdevxdebug.desktop DESTINATION ${SERVICES_INSTALL_DIR})

set(connectiontest_SRCS
    tests/connectiontest.cpp
)
ecm_add_test(${connectiontest_SRCS}
    ${kdevxdebugcore_PART_SRCS}
    TEST_NAME connectiontest
    LINK_LIBRARIES
        KDev::Interfaces
        KDev::Tests
        KDev::Debugger
        KDev::OutputView
        KDev::Shell
        KDev::Util
        Qt5::Test
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
