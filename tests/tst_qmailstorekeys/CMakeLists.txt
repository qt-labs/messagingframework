add_executable(tst_qmailstorekeys tst_qmailstorekeys.cpp)
target_include_directories(tst_qmailstorekeys
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailstorekeys
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailstorekeys COMMAND tst_qmailstorekeys)
set_tests_properties(tst_qmailstorekeys PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailstorekeys
  RUNTIME DESTINATION /opt/tests/qmf)
