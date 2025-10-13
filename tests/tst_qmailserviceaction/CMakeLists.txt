add_executable(tst_qmailserviceaction tst_qmailserviceaction.cpp)
target_include_directories(tst_qmailserviceaction
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailserviceaction
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_qmailserviceaction
  RUNTIME DESTINATION /opt/tests/qmf)
