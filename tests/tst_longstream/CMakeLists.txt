add_executable(tst_longstream tst_longstream.cpp)
target_include_directories(tst_longstream
  PRIVATE
  ../../src/libraries/qmfmessageserver
  ../../src/libraries/qmfclient
  ../../src/libraries/qmfclient/support)
target_link_libraries(tst_longstream
  PRIVATE Qt6::Core Qt6::Test QmfMessageServer)

add_test(NAME tst_longstream COMMAND tst_longstream)
set_tests_properties(tst_longstream PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_longstream
  RUNTIME DESTINATION /opt/tests/qmf)
