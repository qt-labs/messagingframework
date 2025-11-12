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

add_library(pop MODULE ${SRC} ${HEADERS})

target_include_directories(pop
  PRIVATE ../../../libraries/qmfclient ../../../libraries/qmfclient/support ../../../libraries/qmfmessageserver)
target_link_libraries(pop
  PRIVATE Qt6::Core Qt6::Network QmfClient QmfMessageServer)

install(TARGETS pop
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/plugins/messagingframework/messageservices)

if(BUILD_TESTING)
  add_custom_target(link_pop ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink pop/libpop.so ../libpop.so
    DEPENDS pop)
endif()
