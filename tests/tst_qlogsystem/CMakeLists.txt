add_executable(tst_qlogsystem tst_qlogsystem.cpp)
target_include_directories(tst_qlogsystem
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qlogsystem
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_qlogsystem COMMAND tst_qlogsystem)
set_tests_properties(tst_qlogsystem PROPERTIES
  ENVIRONMENT "QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH};LC_NUMERIC=C")

install(TARGETS tst_qlogsystem
  RUNTIME DESTINATION /opt/tests/qmf)
