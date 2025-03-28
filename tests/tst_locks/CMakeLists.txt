add_executable(tst_locks tst_locks.cpp)
target_include_directories(tst_locks
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_locks
  PRIVATE Qt6::Core Qt6::Test QmfClient)

add_test(NAME tst_locks COMMAND tst_locks)
set_tests_properties(tst_locks PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(TARGETS tst_locks
  RUNTIME DESTINATION /opt/tests/qmf)
