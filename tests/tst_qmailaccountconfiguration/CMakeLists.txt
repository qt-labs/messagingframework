add_executable(tst_qmailaccountconfiguration tst_qmailaccountconfiguration.cpp)
target_include_directories(tst_qmailaccountconfiguration
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailaccountconfiguration
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailaccountconfiguration COMMAND tst_qmailaccountconfiguration)
set_tests_properties(tst_qmailaccountconfiguration PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailaccountconfiguration
  RUNTIME DESTINATION /opt/tests/qmf)
