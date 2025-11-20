cmake_minimum_required(VERSION 3.16)

project(QtMessagingFramework
  VERSION 4.0.4
  DESCRIPTION "a C++ library and daemon server process to build email clients")

set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)

option(BUILD_WIDGETS "Build widgets" OFF)
option(USE_HTML_PARSER "Use HTML parser to handle rich text" OFF)
option(USE_ACCOUNTS_QT "Use libaccounts-qt to handle mail account" OFF)

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
if(USE_ACCOUNTS_QT)
  pkg_check_modules(LIBACCOUNTS REQUIRED accounts-qt6 IMPORTED_TARGET)
endif()
if(BUILD_TESTING)
  find_package(Qt6 REQUIRED COMPONENTS Test)
endif()
pkg_check_modules(ICU icu-i18n IMPORTED_TARGET)

# there's probably some better compiler agnostic way to enable more warnings, but this is a start
add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
add_subdirectory(benchmarks)
add_subdirectory(examples)

# TODO: translation support is missing quite a bit. Defining here just some basic ones
# in case someone feels like getting them working some day
if (TARGET Qt6::LinguistTools)
  qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de en_US)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
