add_executable(tst_qmailthread tst_qmailthread.cpp)
target_include_directories(tst_qmailthread
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailthread
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailthread
  RUNTIME DESTINATION /opt/tests/qmf)
