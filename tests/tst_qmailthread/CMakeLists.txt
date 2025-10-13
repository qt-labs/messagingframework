add_executable(tst_qmailthread tst_qmailthread.cpp)
target_include_directories(tst_qmailthread
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailthread
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailthread COMMAND tst_qmailthread)
set_tests_properties(tst_qmailthread PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailthread
  RUNTIME DESTINATION /opt/tests/qmf)
