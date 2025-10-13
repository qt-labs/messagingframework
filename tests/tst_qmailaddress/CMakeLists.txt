add_executable(tst_qmailaddress tst_qmailaddress.cpp)
target_include_directories(tst_qmailaddress
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailaddress
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailaddress COMMAND tst_qmailaddress)
set_tests_properties(tst_qmailaddress PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailaddress
  RUNTIME DESTINATION /opt/tests/qmf)
