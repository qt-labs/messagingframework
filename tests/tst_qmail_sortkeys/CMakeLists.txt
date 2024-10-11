add_executable(tst_qmail_sortkeys tst_qmail_sortkeys.cpp)
target_include_directories(tst_qmail_sortkeys
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmail_sortkeys
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_qmail_sortkeys
  RUNTIME DESTINATION /opt/tests/qmf)
