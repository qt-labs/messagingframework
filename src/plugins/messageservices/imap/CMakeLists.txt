set(SRC
  imapclient.cpp
  imapconfiguration.cpp
  imapprotocol.cpp
  imapservice.cpp
  imapstructure.cpp
  imapauthenticator.cpp
  imapstrategy.cpp
  integerregion.cpp
  imaptransport.cpp
  serviceactionqueue.cpp
  idlenetworksession.cpp)

set(HEADERS
  imapclient.h
  imapconfiguration.h
  imapmailboxproperties.h
  imapprotocol.h
  imapservice.h
  imapstructure.h
  imapauthenticator.h
  imapstrategy.h
  integerregion.h
  imaptransport.h
  serviceactionqueue.h
  idlenetworksession.h)

if (BUILD_WIDGETS)
  set(SRC ${SRC} imapsettings.cpp)
  set(HEADERS ${HEADERS} imapsettings.h)
  qt_add_resources(SRC imap.qrc)
  qt_wrap_ui(SRC imapsettings.ui)
endif()

add_library(imap MODULE ${SRC} ${HEADERS})

target_include_directories(imap
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support ../../../libraries/qmfmessageserver)
target_link_libraries(imap
  PRIVATE Qt6::Core Qt6::Network QmfClient QmfMessageServer)
if (BUILD_WIDGETS)
  target_link_libraries(imap PUBLIC QmfWidgets)
  target_include_directories(imap
  PRIVATE ../../../libraries/qmfwidgets)
else()
  target_compile_definitions(imap PRIVATE QMF_NO_WIDGETS)
endif()

install(TARGETS imap
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt5/plugins/messageservices)

if(BUILD_TESTING)
  add_custom_target(link_imap ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink imap/libimap.so ../libimap.so
    DEPENDS imap)
endif()
