add_executable(tst_qprivateimplementation tst_qprivateimplementation.cpp)
target_include_directories(tst_qprivateimplementation
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qprivateimplementation
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qprivateimplementation COMMAND tst_qprivateimplementation)
set_tests_properties(tst_qprivateimplementation PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qprivateimplementation
  RUNTIME DESTINATION /opt/tests/qmf)
