add_executable(tst_qmailstore tst_qmailstore.cpp)
target_include_directories(tst_qmailstore
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailstore
  PRIVATE Qt6::Core Qt6::Sql Qt6::Test QmfClient)

install(TARGETS tst_qmailstore
  RUNTIME DESTINATION /opt/tests/qmf)
