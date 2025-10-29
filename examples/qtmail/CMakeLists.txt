set(HEADERS
  emailclient.h
  messagelistview.h
  searchview.h
  selectcomposerwidget.h
  readmail.h
  writemail.h
  accountsettings.h
  editaccount.h
  statusmonitorwidget.h
  statusbar.h
  statusmonitor.h
  qmailcomposer.h
  qmailviewer.h
  attachmentlistwidget.h
  detailspage_p.h
  emailcomposer.h
  attachmentoptions.h
  browserwidget.h
  genericviewer.h)

set(SRC
  emailclient.cpp
  messagelistview.cpp
  searchview.cpp
  selectcomposerwidget.cpp
  readmail.cpp
  writemail.cpp
  accountsettings.cpp
  editaccount.cpp
  statusmonitorwidget.cpp
  statusbar.cpp
  statusmonitor.cpp
  qmailcomposer.cpp
  qmailviewer.cpp
  attachmentlistwidget.cpp
  detailspage.cpp
  emailcomposer.cpp
  attachmentoptions.cpp
  browserwidget.cpp
  genericviewer.cpp)

qt_add_resources(SRC qtmail.qrc)
qt_wrap_ui(SRC searchviewbasephone.ui)

add_library(QtMail ${SRC} ${HEADERS})
target_include_directories(QtMail
  PUBLIC
  ../../src/libraries/qmfclient
  ../../src/libraries/qmfclient/support
  ../../src/libraries/qmfmessageserver
  ../../src/libraries/qmfwidgets)
target_link_libraries(QtMail
  PUBLIC Qt6::Core Qt6::Widgets QmfClient QmfMessageServer QmfWidgets)

add_executable(qtmail main.cpp)
target_link_libraries(qtmail
  PUBLIC Qt6::Core QtMail)
