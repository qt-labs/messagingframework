cmake_minimum_required(VERSION 3.16)

project(QtMessagingFramework
  VERSION 4.0.4
  DESCRIPTION "a C++ library and daemon server process to build email clients")

set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)

option(BUILD_WIDGETS "Build widgets" OFF)
option(BUILD_MESSAGESERVER_PLUGINS "Build plugins for the message server" ON)
option(USE_HTML_PARSER "Use HTML parser to handle rich text" OFF)

include(FeatureSummary)
include(GNUInstallDirs)
include(CTest)

find_package(PkgConfig REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Core Xml DBus Sql Network Core5Compat OPTIONAL_COMPONENTS LinguistTools)
if(BUILD_WIDGETS)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()
if(USE_HTML_PARSER)
find_package(Qt6 REQUIRED COMPONENTS Gui)
endif()
if(BUILD_TESTING)
  find_package(Qt6 REQUIRED COMPONENTS Test)
endif()
pkg_check_modules(ICU icu-i18n IMPORTED_TARGET)

add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
add_subdirectory(benchmarks)
add_subdirectory(examples)

if (TARGET Qt6::LinguistTools)
  qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES ar de en_GB en_SU en_US es fr it ja ko pt_BR zh_CN zh_TW)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
