add_executable(tst_locks tst_locks.cpp)
target_include_directories(tst_locks
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_locks
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_locks
  RUNTIME DESTINATION /opt/tests/qmf)
