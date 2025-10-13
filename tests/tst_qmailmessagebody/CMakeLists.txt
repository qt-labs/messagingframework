add_executable(tst_qmailmessagebody tst_qmailmessagebody.cpp)
target_include_directories(tst_qmailmessagebody
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessagebody
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailmessagebody COMMAND tst_qmailmessagebody)
set_tests_properties(tst_qmailmessagebody PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailmessagebody
  RUNTIME DESTINATION /opt/tests/qmf)
