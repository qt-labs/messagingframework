add_executable(tst_qmailnamespace tst_qmailnamespace.cpp)
target_include_directories(tst_qmailnamespace
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailnamespace
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmailnamespace COMMAND tst_qmailnamespace)
set_tests_properties(tst_qmailnamespace PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailnamespace
  RUNTIME DESTINATION /opt/tests/qmf)
