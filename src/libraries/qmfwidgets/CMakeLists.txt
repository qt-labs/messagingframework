set(SRC
  emailfoldermodel.cpp
  emailfolderview.cpp
  folderdelegate.cpp
  foldermodel.cpp
  folderview.cpp
  selectfolder.cpp
  qtmailnamespace.cpp)

set(HEADERS
  emailfoldermodel.h
  emailfolderview.h
  folderdelegate.h
  foldermodel.h
  folderview.h
  selectfolder.h
  qtmailnamespace.h)

qt_add_resources(SRC qmfutil.qrc)

add_library(QmfWidgets SHARED ${SRC} ${HEADERS})

target_compile_definitions(QmfWidgets PRIVATE QMFUTIL_INTERNAL)
target_include_directories(QmfWidgets PRIVATE ../qmfclient ../qmfclient/support)
target_link_libraries(QmfWidgets
  PUBLIC Qt6::Core Qt6::Widgets QmfClient)
set_target_properties(QmfWidgets PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  PUBLIC_HEADER "${HEADERS}")

install(TARGETS QmfWidgets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt6/QmfWidgets)

if (TARGET Qt6::LinguistTools)
  qt_add_translations(libqmfwidgets)
endif()

