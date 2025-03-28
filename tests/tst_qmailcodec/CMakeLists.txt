add_executable(tst_qmailcodec tst_qmailcodec.cpp)
target_include_directories(tst_qmailcodec
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailcodec
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qmailcodec COMMAND tst_qmailcodec)
set_tests_properties(tst_qmailcodec PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmailcodec
  RUNTIME DESTINATION /opt/tests/qmf)
