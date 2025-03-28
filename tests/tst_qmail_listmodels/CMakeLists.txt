add_executable(tst_qmail_listmodels tst_qmail_listmodels.cpp)
target_include_directories(tst_qmail_listmodels
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmail_listmodels
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qmail_listmodels COMMAND tst_qmail_listmodels)
set_tests_properties(tst_qmail_listmodels PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmail_listmodels
  RUNTIME DESTINATION /opt/tests/qmf)
