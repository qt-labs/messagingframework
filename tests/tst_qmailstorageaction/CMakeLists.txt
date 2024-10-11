add_executable(tst_qmailstorageaction tst_qmailstorageaction.cpp)
target_include_directories(tst_qmailstorageaction
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailstorageaction
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailstorageaction
  RUNTIME DESTINATION /opt/tests/qmf)
