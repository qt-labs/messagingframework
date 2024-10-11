add_executable(tst_longstring tst_longstring.cpp)
target_include_directories(tst_longstring
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_longstring
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_longstring
  RUNTIME DESTINATION /opt/tests/qmf)
