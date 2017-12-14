cmake_minimum_required(VERSION 3.0.0)
project(xdebug)

find_package (ECM "5.14.0" REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(FeatureSummary)

set(QT_MIN_VERSION "5.5.0")
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Test)
set(KF5_DEP_VERSION "5.15.0")
find_package(KF5 ${KF5_DEP_VERSION} REQUIRED COMPONENTS
    TextEditor
    I18n
    ItemModels # needed because missing in KDevPlatformConfig.cmake, remove once dep on kdevplatform >=5.2.2
)

find_package(KDevPlatform 5.1 CONFIG)
set_package_properties(KDevPlatform PROPERTIES
    TYPE REQUIRED
)

# TODO: That plugin should install a CMake config file
find_path(EXECUTEBROWSER_INCLUDE_DIR iexecutebrowserplugin.h PATH_SUFFIXES kdevexecutebrowser)
include_directories(${EXECUTEBROWSER_INCLUDE_DIR})

add_definitions(-DTRANSLATION_DOMAIN=\"kdevxdebug\")

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

ki18n_wrap_ui(kdevxdebug_PART_SRCS
    launchconfigurationpage.ui
)

ecm_qt_declare_logging_category(kdevxdebugcore_PART_SRCS
    HEADER debuggerdebug.h
    IDENTIFIER KDEV_PHP_DEBUGGER
    CATEGORY_NAME "kdevelop.languages.php.debugger"
)


kdevplatform_add_plugin(kdevxdebug JSON kdevxdebug.json SOURCES ${kdevxdebug_PART_SRCS} ${kdevxdebugcore_PART_SRCS})

target_link_libraries(kdevxdebug
    KDev::Interfaces
    KDev::Debugger
    KDev::OutputView
    KDev::Shell
    KDev::Util
)

if (BUILD_TESTING AND TARGET KDev::Tests)
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
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
