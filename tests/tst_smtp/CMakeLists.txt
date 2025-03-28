set(SMTP_SOURCES
  ../../src/plugins/messageservices/smtp/smtpauthenticator.h
  ../../src/plugins/messageservices/smtp/smtpclient.h
  ../../src/plugins/messageservices/smtp/smtpconfiguration.h
  ../../src/plugins/messageservices/smtp/smtpclient.cpp
  ../../src/plugins/messageservices/smtp/smtpauthenticator.cpp
  ../../src/plugins/messageservices/smtp/smtpconfiguration.cpp)

add_executable(tst_smtp tst_smtp.cpp ${SMTP_SOURCES})
target_include_directories(tst_smtp
  PRIVATE
  ../../src/libraries/qmfmessageserver
  ../../src/libraries/qmfclient
  ../../src/libraries/qmfclient/support
  ../../src/plugins/messageservices/smtp)
target_link_libraries(tst_smtp
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Network Qt6::Test QmfClient QmfMessageServer)

add_test(NAME tst_smtp COMMAND tst_smtp)
set_tests_properties(tst_smtp PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})
set_tests_properties(tst_smtp PROPERTIES WILL_FAIL TRUE)

install(TARGETS tst_smtp
  RUNTIME DESTINATION /opt/tests/qmf)
