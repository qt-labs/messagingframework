add_executable(tst_qmailstorekeys tst_qmailstorekeys.cpp)
target_include_directories(tst_qmailstorekeys
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailstorekeys
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailstorekeys
  RUNTIME DESTINATION /opt/tests/qmf)
