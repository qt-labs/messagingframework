set(SRC
  longstring.cpp
  locks.cpp
  qmailaccount.cpp
  qmailaccountconfiguration.cpp
  qmailaccountkey.cpp
  qmailaccountlistmodel.cpp
  qmailaccountsortkey.cpp
  qmailaction.cpp
  qmailaddress.cpp
  qmailcodec.cpp
  qmailcontentmanager.cpp
  qmailcrypto.cpp
  qmaildatacomparator.cpp
  qmaildisconnected.cpp
  qmailfolder.cpp
  qmailfolderkey.cpp
  qmailfoldersortkey.cpp
  qmailid.cpp
  qmailinstantiations.cpp
  qmailkeyargument.cpp
  qmailmessage.cpp
  qmailmessagekey.cpp
  qmailmessagelistmodel.cpp
  qmailmessagemodelbase.cpp
  qmailmessageremovalrecord.cpp
  qmailmessageserver.cpp
  qmailmessageset.cpp
  qmailmessagesortkey.cpp
  qmailmessagethreadedmodel.cpp
  qmailserviceaction.cpp
  qmailstore.cpp
  qmailstoreaccount.cpp
  qmailstore_p.cpp
  qmailstoreimplementation_p.cpp
  qmailstorenotifier_p.cpp
  qmailstoresql_p.cpp
  qmailtimestamp.cpp
  qmailthread.cpp
  qmailthreadkey.cpp
  qmailthreadlistmodel.cpp
  qmailthreadsortkey.cpp
  qprivateimplementation.cpp
  support/qmailnamespace.cpp
  support/qmaillog.cpp
  support/qmailpluginmanager.cpp)

set(PUBLIC_HEADERS
  qmailaccount.h
  qmailaccountconfiguration.h
  qmailaccountkey.h
  qmailaccountlistmodel.h
  qmailaccountsortkey.h
  qmailaction.h
  qmailaddress.h
  qmailcodec.h
  qmailcontentmanager.h
  qmailcrypto.h
  qmailcryptofwd.h
  qmaildatacomparator.h
  qmaildisconnected.h
  qmailfolder.h
  qmailfolderfwd.h
  qmailfolderkey.h
  qmailfoldersortkey.h
  qmailid.h
  qmailkeyargument.h
  qmailmessage.h
  qmailmessagefwd.h
  qmailmessagekey.h
  qmailmessagelistmodel.h
  qmailmessagemodelbase.h
  qmailmessageremovalrecord.h
  qmailmessageserver.h
  qmailmessageset.h
  qmailmessagesortkey.h
  qmailmessagethreadedmodel.h
  qmailserviceaction.h
  qmailsortkeyargument.h
  qmailstore.h
  qmailstoreaccount.h
  qmailtimestamp.h
  qmailthread.h
  qmailthreadkey.h
  qmailthreadlistmodel.h
  qmailthreadsortkey.h
  qprivateimplementation.h
  support/qmailglobal.h
  support/qmaillog.h
  support/qmailnamespace.h
  support/qmailpluginmanager.h
  support/qmailipc.h)

set(PRIVATE_HEADERS
  qprivateimplementationdef_p.h
  locks_p.h
  mailkeyimpl_p.h
  mailsortkeyimpl_p.h
  qmailaccountkey_p.h
  qmailaccountsortkey_p.h
  qmailfolderkey_p.h
  qmailfoldersortkey_p.h
  qmailmessage_p.h
  qmailmessagekey_p.h
  qmailmessageset_p.h
  qmailmessagesortkey_p.h
  qmailserviceaction_p.h
  qmailstore_p.h
  qmailstoreimplementation_p.h
  qmailstorenotifier_p.h
  qmailstoresql_p.h
  qmailthreadkey_p.h
  qmailthreadsortkey_p.h
  longstring_p.h)

if (ICU_FOUND)
  set(SRC ${SRC}
    support/qmailcharsetdetector.cpp)
  set(PRIVATE_HEADERS ${PRIVATE_HEADERS}
    support/qmailcharsetdetector_p.h
    support/qmailcharsetdetector.h)
endif()

if (USE_ACCOUNTS_QT)
  set(SRC ${SRC} libaccounts_p.cpp)
  set(PRIVATE_HEADERS ${PRIVATE_HEADERS} libaccounts_p.h)
endif()

qt_add_resources(SRC qmf.qrc qmf_qt.qrc)

set_source_files_properties(qmailservice.xml PROPERTIES
  INCLUDE qmailserviceaction.h)
qt_add_dbus_interface(GENERATED_SOURCES qmailservice.xml qmailservice_interface)
set_source_files_properties(qmailstore.xml PROPERTIES
  INCLUDE qmailstore.h)
qt_add_dbus_adaptor(GENERATED_SOURCES qmailstore.xml qmailstorenotifier_p.h QMailStoreNotifier qmailstore_adaptor)

add_library(QmfClient SHARED ${SRC} ${GENERATED_SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})

target_compile_definitions(QmfClient PRIVATE QMF_INTERNAL)
target_include_directories(QmfClient PRIVATE support)
target_link_libraries(QmfClient
  PRIVATE Qt6::Sql Qt6::Network Qt6::Core5Compat
  PUBLIC Qt6::Core Qt6::DBus)
set_target_properties(QmfClient PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  PUBLIC_HEADER "${PUBLIC_HEADERS}"
  PRIVATE_HEADER "${PRIVATE_HEADERS}")

if (ICU_FOUND)
  target_compile_definitions(QmfClient PRIVATE HAVE_LIBICU)
  target_link_libraries(QmfClient PRIVATE PkgConfig::ICU)
endif()

if (USE_HTML_PARSER)
  target_compile_definitions(QmfClient PRIVATE USE_HTML_PARSER)
  target_link_libraries(QmfClient
    PRIVATE Qt6::Gui)
endif()

if (USE_ACCOUNTS_QT)
  target_compile_definitions(QmfClient PRIVATE USE_ACCOUNTS_QT)
  target_link_libraries(QmfClient PRIVATE Qt6::Xml PkgConfig::LIBACCOUNTS)
  set(ACCOUNT_MANAGER "LibAccountManager")
endif()

if (DEFINED ACCOUNT_MANAGER AND NOT ACCOUNT_MANAGER STREQUAL "")
  message(STATUS "Using '${ACCOUNT_MANAGER}' as the account manager.")
  target_compile_definitions(QmfClient PRIVATE QMF_ACCOUNT_MANAGER_CLASS=${ACCOUNT_MANAGER})
endif()

install(TARGETS QmfClient
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt5/QmfClient
  PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt5/QmfClient/private)

if (TARGET Qt6::LinguistTools)
  qt_add_translations(libqtopiamail)
endif()
