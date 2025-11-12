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

add_library(smtp MODULE ${SRC} ${HEADERS})

target_include_directories(smtp
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support ../../../libraries/qmfmessageserver)
target_link_libraries(smtp
  PRIVATE Qt6::Core Qt6::Network QmfClient QmfMessageServer)

install(TARGETS smtp
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/messagingframework/messageservices)

if(BUILD_TESTING)
  add_custom_target(link_smtp ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink smtp/libsmtp.so ../libsmtp.so
    DEPENDS smtp)
endif()
