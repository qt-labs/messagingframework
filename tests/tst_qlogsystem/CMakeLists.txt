add_executable(tst_qlogsystem tst_qlogsystem.cpp)
target_include_directories(tst_qlogsystem
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qlogsystem
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_qlogsystem
  RUNTIME DESTINATION /opt/tests/qmf)
