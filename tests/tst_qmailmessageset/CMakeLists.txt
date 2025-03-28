add_executable(tst_qmailmessageset tst_qmailmessageset.cpp)
target_include_directories(tst_qmailmessageset
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessageset
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qmailmessageset COMMAND tst_qmailmessageset)
set_tests_properties(tst_qmailmessageset PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailmessageset
  RUNTIME DESTINATION /opt/tests/qmf)
