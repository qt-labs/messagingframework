add_executable(tst_qmail_sortkeys tst_qmail_sortkeys.cpp)
target_include_directories(tst_qmail_sortkeys
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmail_sortkeys
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmail_sortkeys COMMAND tst_qmail_sortkeys)
set_tests_properties(tst_qmail_sortkeys PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmail_sortkeys
  RUNTIME DESTINATION /opt/tests/qmf)
