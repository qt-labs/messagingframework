add_executable(tst_qmail_listmodels tst_qmail_listmodels.cpp)
target_include_directories(tst_qmail_listmodels
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_qmail_listmodels
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_qmail_listmodels
  RUNTIME DESTINATION /opt/tests/qmf)
