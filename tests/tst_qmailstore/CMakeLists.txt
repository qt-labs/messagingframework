add_executable(tst_qmailstore tst_qmailstore.cpp)
target_include_directories(tst_qmailstore
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailstore
  PRIVATE Qt6::Core Qt6::Sql Qt6::Test QmfClient)

add_test(NAME tst_qmailstore COMMAND tst_qmailstore)
set_tests_properties(tst_qmailstore PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailstore
  RUNTIME DESTINATION /opt/tests/qmf)
