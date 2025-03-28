add_executable(tst_qmailmessagepart tst_qmailmessagepart.cpp)
target_include_directories(tst_qmailmessagepart
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessagepart
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qmailmessagepart COMMAND tst_qmailmessagepart)
set_tests_properties(tst_qmailmessagepart PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailmessagepart
  RUNTIME DESTINATION /opt/tests/qmf)
