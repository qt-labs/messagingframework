set(SRC
  imapclient.cpp
  imapconfiguration.cpp
  imaplog.cpp
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
  imaplog.h
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

add_library(imap MODULE ${SRC} ${HEADERS})

target_include_directories(imap
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support ../../../libraries/qmfmessageserver)
target_link_libraries(imap
  PRIVATE Qt6::Core Qt6::Network QmfClient QmfMessageServer)

install(TARGETS imap
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/messagingframework/messageservices)

if(BUILD_TESTING)
  add_custom_target(link_imap ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink imap/libimap.so ../libimap.so
    DEPENDS imap)
endif()
