add_executable(tst_qmaildisconnected tst_qmaildisconnected.cpp)
target_include_directories(tst_qmaildisconnected
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmaildisconnected
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qmaildisconnected COMMAND tst_qmaildisconnected)
set_tests_properties(tst_qmaildisconnected PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmaildisconnected
  RUNTIME DESTINATION /opt/tests/qmf)
