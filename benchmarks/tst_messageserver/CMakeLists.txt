set(QMF_SOURCES
  ../../src/plugins/messageservices/imap/imaplog.h
  ../../src/plugins/messageservices/imap/imaplog.cpp
  ../../src/plugins/messageservices/imap/imapconfiguration.h
  ../../src/plugins/messageservices/imap/imapconfiguration.cpp)

add_executable(tst_messageserver
  benchmarkcontext.h
  qscopedconnection.h
  testfsusage.h
  testmalloc.h
  3rdparty/cycle_p.h
  benchmarkcontext.cpp
  qscopedconnection.cpp
  testfsusage.cpp
  testmalloc.cpp
  tst_messageserver.cpp
  ${QMF_SOURCES})
target_include_directories(tst_messageserver
  PRIVATE
  ../../src/tools/messageserver
  ../../src/plugins/messageservices/imap)
target_link_libraries(tst_messageserver
  PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test QmfClient MessageServer)

install(TARGETS tst_messageserver
  RUNTIME DESTINATION /opt/tests/qmf)
