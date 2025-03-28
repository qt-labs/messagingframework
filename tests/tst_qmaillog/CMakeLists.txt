add_executable(tst_qmaillog tst_qmaillog.cpp)
target_include_directories(tst_qmaillog
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmaillog
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qmaillog COMMAND tst_qmaillog)
set_tests_properties(tst_qmaillog PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmaillog
  RUNTIME DESTINATION /opt/tests/qmf)
