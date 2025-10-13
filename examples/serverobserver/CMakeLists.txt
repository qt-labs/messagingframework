set(SRC
  serverobserver.h
  serverobserver.cpp
  main.cpp)

add_executable(serverobserver5 ${SRC})
target_include_directories(serverobserver5
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(serverobserver5
  PRIVATE Qt6::Core Qt6::Test QmfClient QmfMessageServer)
