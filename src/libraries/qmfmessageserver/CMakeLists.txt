set(SRC
  qmailauthenticator.cpp
  qmailmessagebuffer.cpp
  qmailmessageclassifier.cpp
  qmailmessageservice.cpp
  qmailserviceconfiguration.cpp
  qmailstoreaccountfilter.cpp
  qmailtransport.cpp
  qmailheartbeattimer_qtimer.cpp) # NB: There are multiple implementations

set(HEADERS
  qmailauthenticator.h
  qmailmessagebuffer.h
  qmailmessageclassifier.h
  qmailmessageservice.h
  qmailserviceconfiguration.h
  qmailstoreaccountfilter.h
  qmailtransport.h
  qmailheartbeattimer.h)

if (BUILD_MESSAGESERVER_PLUGINS)
  set(SRC ${SRC}
    qmailmessageserverplugin.cpp)
  set(HEADERS ${HEADERS}
    qmailmessageserverplugin.h)
endif()

add_library(QmfMessageServer SHARED ${SRC} ${HEADERS})

if (BUILD_WIDGETS)
  target_link_libraries(QmfMessageServer PUBLIC Qt6::Widgets)
else()
  target_compile_definitions(QmfMessageServer PRIVATE QMF_NO_WIDGETS)
endif()

target_compile_definitions(QmfMessageServer PRIVATE MESSAGESERVER_INTERNAL)
target_include_directories(QmfMessageServer PRIVATE ../qmfclient ../qmfclient/support)
target_link_libraries(QmfMessageServer
  PRIVATE Qt6::Network
  PUBLIC Qt6::Core QmfClient)
set_target_properties(QmfMessageServer PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  PUBLIC_HEADER "${HEADERS}")

install(TARGETS QmfMessageServer
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt5/QmfMessageServer)
