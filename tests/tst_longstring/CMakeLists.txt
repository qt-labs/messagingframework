add_executable(tst_longstring tst_longstring.cpp)
target_include_directories(tst_longstring
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_longstring
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_longstring COMMAND tst_longstring)
set_tests_properties(tst_longstring PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_longstring
  RUNTIME DESTINATION /opt/tests/qmf)
