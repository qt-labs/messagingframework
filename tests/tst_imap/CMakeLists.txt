set(IMAP_SOURCES
  ../../src/plugins/messageservices/imap/imapauthenticator.h
  ../../src/plugins/messageservices/imap/imapclient.h
  ../../src/plugins/messageservices/imap/imapprotocol.h
  ../../src/plugins/messageservices/imap/imaptransport.h
  ../../src/plugins/messageservices/imap/imapstrategy.h
  ../../src/plugins/messageservices/imap/imapstructure.h
  ../../src/plugins/messageservices/imap/integerregion.h
  ../../src/plugins/messageservices/imap/imapconfiguration.h
  ../../src/plugins/messageservices/imap/imapclient.cpp
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

install(TARGETS tst_imap
  RUNTIME DESTINATION /opt/tests/qmf)
