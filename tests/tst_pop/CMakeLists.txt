set(POP_SOURCES
  ../../src/plugins/messageservices/pop/popauthenticator.h
  ../../src/plugins/messageservices/pop/popclient.h
  ../../src/plugins/messageservices/pop/poplog.h
  ../../src/plugins/messageservices/pop/popconfiguration.h
  ../../src/plugins/messageservices/pop/popclient.cpp
  ../../src/plugins/messageservices/pop/poplog.cpp
  ../../src/plugins/messageservices/pop/popauthenticator.cpp
  ../../src/plugins/messageservices/pop/popconfiguration.cpp)

add_executable(tst_pop tst_pop.cpp ${POP_SOURCES})
target_include_directories(tst_pop
  PRIVATE
  ../../src/libraries/qmfmessageserver
  ../../src/libraries/qmfclient
  ../../src/libraries/qmfclient/support
  ../../src/plugins/messageservices/pop)
target_link_libraries(tst_pop
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Network Qt6::Test QmfClient QmfMessageServer)

add_test(NAME tst_pop COMMAND tst_pop)
set_tests_properties(tst_pop PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

# until we figure out how to run a test POP server
set_tests_properties(tst_pop PROPERTIES WILL_FAIL TRUE)

install(TARGETS tst_pop
  RUNTIME DESTINATION /opt/tests/qmf)
