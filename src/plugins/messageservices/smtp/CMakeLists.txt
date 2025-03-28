set(SRC
  smtpauthenticator.cpp
  smtpclient.cpp
  smtpconfiguration.cpp
  smtpservice.cpp)

set(HEADERS
  smtpauthenticator.h
  smtpclient.h
  smtpconfiguration.h
  smtpservice.h)

if (BUILD_WIDGETS)
  set(SRC ${SRC} smtpsettings.cpp)
  set(HEADERS ${HEADERS} smtpsettings.h)
  qt_wrap_ui(SRC smtpsettings.ui)
endif()

add_library(smtp MODULE ${SRC} ${HEADERS})

target_include_directories(smtp
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support ../../../libraries/qmfmessageserver)
target_link_libraries(smtp
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Network QmfClient QmfMessageServer)
if (BUILD_WIDGETS)
  target_link_libraries(smtp PUBLIC QmfWidgets)
  target_include_directories(smtp
    PRIVATE ../../../libraries/qmfwidgets)
else()
  target_compile_definitions(smtp PRIVATE QMF_NO_WIDGETS)
endif()

install(TARGETS smtp
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt5/plugins/messageservices)

if(BUILD_TESTING)
  add_custom_target(link_smtp ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink smtp/libsmtp.so ../libsmtp.so
    DEPENDS smtp)
endif()
