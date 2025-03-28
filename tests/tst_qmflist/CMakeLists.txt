add_executable(tst_qmflist tst_qmflist.cpp)
target_include_directories(tst_qmflist
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmflist
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

add_test(NAME tst_qmflist COMMAND tst_qmflist)
set_tests_properties(tst_qmflist PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_qmflist
  RUNTIME DESTINATION /opt/tests/qmf)
