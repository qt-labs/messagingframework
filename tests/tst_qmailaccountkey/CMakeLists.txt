add_executable(tst_qmailaccountkey tst_qmailaccountkey.cpp)
target_include_directories(tst_qmailaccountkey
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailaccountkey
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailaccountkey COMMAND tst_qmailaccountkey)
set_tests_properties(tst_qmailaccountkey PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailaccountkey
  RUNTIME DESTINATION /opt/tests/qmf)
