set(SRC
  messageserver.cpp
  newcountnotifier.cpp
  servicehandler.cpp)

set(HEADERS
  messageserver.h
  servicehandler.h
  newcountnotifier.h)

qt_add_dbus_adaptor(GENERATED_SOURCES ../../libraries/qmfclient/qmailservice.xml servicehandler.h ServiceHandler qmailservice_adaptor)

add_library(MessageServer ${SRC} ${GENERATED_SOURCES} ${HEADERS})

if (BUILD_WIDGETS)
  target_link_libraries(MessageServer
    PRIVATE Qt6::Widgets)
else()
  target_compile_definitions(MessageServer PRIVATE QMF_NO_WIDGETS)
endif()
target_include_directories(MessageServer PUBLIC
  ../../libraries/qmfmessageserver
  ../../libraries/qmfclient
  ../../libraries/qmfclient/support)
target_link_libraries(MessageServer
  PRIVATE Qt6::DBus
  PUBLIC Qt6::Core QmfClient QmfMessageServer)

if (USE_HTML_PARSER)
  target_compile_definitions(MessageServer PRIVATE USE_HTML_PARSER)
  target_link_libraries(MessageServer PRIVATE Qt6::Gui)
endif()

add_executable(messageserver main.cpp)
target_link_libraries(messageserver
  PRIVATE Qt6::Core MessageServer)

if (TARGET Qt6::LinguistTools)
  qt_add_translations(messageserver)
endif()

install(TARGETS messageserver
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
