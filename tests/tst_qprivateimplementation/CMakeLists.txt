add_executable(tst_qprivateimplementation tst_qprivateimplementation.cpp)
target_include_directories(tst_qprivateimplementation
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qprivateimplementation
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qprivateimplementation
  RUNTIME DESTINATION /opt/tests/qmf)
