add_executable(tst_python_email tst_python_email.cpp)
target_include_directories(tst_python_email
  PRIVATE ../../src/libraries/qmfclient ../../src/libraries/qmfclient/support)
target_link_libraries(tst_python_email
  PRIVATE Qt6::Core Qt6::Test QmfClient)

install(TARGETS tst_python_email
  RUNTIME DESTINATION /opt/tests/qmf)

add_test(NAME tst_python_email COMMAND tst_python_email)
set_tests_properties(tst_python_email PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

install(DIRECTORY testdata DESTINATION /opt/tests/qmf)
