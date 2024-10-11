add_executable(tst_qmailmessageheader tst_qmailmessageheader.cpp)
target_include_directories(tst_qmailmessageheader
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessageheader
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailmessageheader
  RUNTIME DESTINATION /opt/tests/qmf)
