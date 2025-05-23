set(SRC
  popclient.cpp
  popconfiguration.cpp
  poplog.cpp
  popservice.cpp
  popauthenticator.cpp)

set(HEADERS
  popclient.h
  popconfiguration.h
  poplog.h
  popservice.h
  popauthenticator.h)

if (BUILD_WIDGETS)
  set(SRC ${SRC} popsettings.cpp)
  set(HEADERS ${HEADERS} popsettings.h)
  qt_wrap_ui(SRC popsettings.ui)
endif()

add_library(pop MODULE ${SRC} ${HEADERS})

target_include_directories(pop
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support ../../../libraries/qmfmessageserver)
target_link_libraries(pop
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Network QmfClient QmfMessageServer)
if (BUILD_WIDGETS)
  target_link_libraries(pop PUBLIC QmfWidgets)
  target_include_directories(pop
    PRIVATE ../../../libraries/qmfwidgets)
else()
  target_compile_definitions(pop PRIVATE QMF_NO_WIDGETS)
endif()

install(TARGETS pop
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt5/plugins/messageservices)

if(BUILD_TESTING)
  add_custom_target(link_pop ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink pop/libpop.so ../libpop.so
    DEPENDS pop)
endif()
