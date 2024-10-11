add_executable(tst_qmflist tst_qmflist.cpp)
target_include_directories(tst_qmflist
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmflist
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmflist
  RUNTIME DESTINATION /opt/tests/qmf)
