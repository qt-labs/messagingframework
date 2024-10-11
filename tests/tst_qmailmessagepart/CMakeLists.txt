add_executable(tst_qmailmessagepart tst_qmailmessagepart.cpp)
target_include_directories(tst_qmailmessagepart
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessagepart
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailmessagepart
  RUNTIME DESTINATION /opt/tests/qmf)
