set(IMAP_SOURCES
  ../../src/plugins/messageservices/imap/imapauthenticator.h
  ../../src/plugins/messageservices/imap/imapclient.h
  ../../src/plugins/messageservices/imap/imaplog.h
  ../../src/plugins/messageservices/imap/imapprotocol.h
  ../../src/plugins/messageservices/imap/imaptransport.h
  ../../src/plugins/messageservices/imap/imapstrategy.h
  ../../src/plugins/messageservices/imap/imapstructure.h
  ../../src/plugins/messageservices/imap/integerregion.h
  ../../src/plugins/messageservices/imap/imapconfiguration.h
  ../../src/plugins/messageservices/imap/imapclient.cpp
  ../../src/plugins/messageservices/imap/imaplog.cpp
  ../../src/plugins/messageservices/imap/imapauthenticator.cpp
  ../../src/plugins/messageservices/imap/imapprotocol.cpp
  ../../src/plugins/messageservices/imap/imaptransport.cpp
  ../../src/plugins/messageservices/imap/imapstrategy.cpp
  ../../src/plugins/messageservices/imap/imapstructure.cpp
  ../../src/plugins/messageservices/imap/integerregion.cpp
  ../../src/plugins/messageservices/imap/imapconfiguration.cpp)

add_executable(tst_imap tst_imap.cpp ${IMAP_SOURCES})
target_include_directories(tst_imap
  PRIVATE
  ../../src/libraries/qmfmessageserver
  ../../src/libraries/qmfclient
  ../../src/libraries/qmfclient/support
  ../../src/plugins/messageservices/imap)
target_link_libraries(tst_imap
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Network Qt6::Test QmfClient QmfMessageServer)

add_test(NAME tst_imap COMMAND tst_imap)
set_tests_properties(tst_imap PROPERTIES
  ENVIRONMENT QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/src/plugins:$ENV{QT_PLUGIN_PATH})

# TODO: figure out how to run an IMAP server. until that this might fail or not.
# at the moment passes so following commented out.
#set_tests_properties(tst_imap PROPERTIES WILL_FAIL TRUE)

install(TARGETS tst_imap
  RUNTIME DESTINATION /opt/tests/qmf)
