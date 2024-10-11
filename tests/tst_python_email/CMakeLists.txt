add_executable(tst_python_email tst_python_email.cpp)
target_include_directories(tst_python_email
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_python_email
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_python_email
  RUNTIME DESTINATION /opt/tests/qmf)

install(DIRECTORY testdata DESTINATION /opt/tests/qmf)
