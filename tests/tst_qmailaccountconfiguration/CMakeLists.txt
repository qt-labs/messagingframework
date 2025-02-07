add_executable(tst_qmailaccountconfiguration tst_qmailaccountconfiguration.cpp)
target_include_directories(tst_qmailaccountconfiguration
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailaccountconfiguration
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_qmailaccountconfiguration
  RUNTIME DESTINATION /opt/tests/qmf)
