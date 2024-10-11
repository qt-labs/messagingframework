add_executable(tst_qmailmessageset tst_qmailmessageset.cpp)
target_include_directories(tst_qmailmessageset
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessageset
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailmessageset
  RUNTIME DESTINATION /opt/tests/qmf)
