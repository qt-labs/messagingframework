add_executable(tst_qmailaddress tst_qmailaddress.cpp)
target_include_directories(tst_qmailaddress
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailaddress
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailaddress
  RUNTIME DESTINATION /opt/tests/qmf)
