add_executable(tst_qmailmessagebody tst_qmailmessagebody.cpp)
target_include_directories(tst_qmailmessagebody
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailmessagebody
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailmessagebody
  RUNTIME DESTINATION /opt/tests/qmf)
