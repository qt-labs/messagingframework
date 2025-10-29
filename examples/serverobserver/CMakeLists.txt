set(SRC
  serverobserver.h
  serverobserver.cpp
  main.cpp)

add_executable(serverobserver ${SRC})
target_include_directories(serverobserver
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(serverobserver
  PRIVATE Qt6::Core Qt6::Test QmfClient QmfMessageServer)
