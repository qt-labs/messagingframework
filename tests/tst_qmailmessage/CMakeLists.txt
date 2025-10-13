add_executable(tst_qmailmessage tst_qmailmessage.cpp)
target_include_directories(tst_qmailmessage
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessage
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailmessage COMMAND tst_qmailmessage)
set_tests_properties(tst_qmailmessage PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailmessage
  RUNTIME DESTINATION /opt/tests/qmf)
