add_executable(tst_longstream tst_longstream.cpp)
target_include_directories(tst_longstream
  PRIVATE
  ../../src/libraries/qmfmessageserver
  ../../src/libraries/qmfclient
  ../../src/libraries/qmfclient/support)
target_link_libraries(tst_longstream
  PRIVATE Qt6::Core Qt6::Test QmfMessageServer)

install(TARGETS tst_longstream
  RUNTIME DESTINATION /opt/tests/qmf)
