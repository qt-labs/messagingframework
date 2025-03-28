add_executable(tst_qmailmessageheader tst_qmailmessageheader.cpp)
target_include_directories(tst_qmailmessageheader
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessageheader
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qmailmessageheader COMMAND tst_qmailmessageheader)
set_tests_properties(tst_qmailmessageheader PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailmessageheader
  RUNTIME DESTINATION /opt/tests/qmf)
