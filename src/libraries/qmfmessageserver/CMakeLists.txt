set(SRC
  longstream.cpp
  qmailauthenticator.cpp
  qmailcredentials.cpp
  qmailmessagebuffer.cpp
  qmailmessageclassifier.cpp
  qmailmessageserverplugin.cpp
  qmailmessageservice.cpp
  qmailserviceconfiguration.cpp
  qmailstoreaccountfilter.cpp
  qmailtransport.cpp
  qmailheartbeattimer_qtimer.cpp) # NB: There are multiple implementations

set(HEADERS
  longstream_p.h
  qmailauthenticator.h
  qmailcredentials.h
  qmailmessagebuffer.h
  qmailmessageclassifier.h
  qmailmessageserverplugin.h
  qmailmessageservice.h
  qmailserviceconfiguration.h
  qmailstoreaccountfilter.h
  qmailtransport.h
  qmailheartbeattimer.h)

add_library(QmfMessageServer SHARED ${SRC} ${HEADERS})

target_compile_definitions(QmfMessageServer PRIVATE MESSAGESERVER_INTERNAL)
target_include_directories(QmfMessageServer PRIVATE ../qmfclient ../qmfclient/support)
target_link_libraries(QmfMessageServer
  PRIVATE Qt6::Network
  PUBLIC Qt6::Core QmfClient)
set_target_properties(QmfMessageServer PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  PUBLIC_HEADER "${HEADERS}"
  CXX_VISIBILITY_PRESET hidden)

install(TARGETS QmfMessageServer
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt6/QmfMessageServer)

if (ECM_FOUND)
  ecm_generate_pkgconfig_file(
        BASE_NAME QmfMessageServer
        INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/qt6/QmfMessageServer
        DEPS Qt6Core QmfClient PRIVATE Qt6Network
        URL "https://codereview.qt-project.org/gitweb?p=qt-labs%2Fmessagingframework.git"
        INSTALL)
endif()
