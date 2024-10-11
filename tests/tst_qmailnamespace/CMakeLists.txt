add_executable(tst_qmailnamespace tst_qmailnamespace.cpp)
target_include_directories(tst_qmailnamespace
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmailnamespace
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient)

install(TARGETS tst_qmailnamespace
  RUNTIME DESTINATION /opt/tests/qmf)
