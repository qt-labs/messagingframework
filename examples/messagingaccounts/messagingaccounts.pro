TEMPLATE = app
target.path += $$QMF_INSTALL_ROOT/bin
TARGET = messagingaccounts5
QT += widgets qmfclient qmfmessageserver qmfwidgets

QTMAIL_EXAMPLE=../qtmail

INCLUDEPATH += \
                 $$QTMAIL_EXAMPLE

HEADERS += $$QTMAIL_EXAMPLE/accountsettings.h \
           $$QTMAIL_EXAMPLE/editaccount.h \
           $$QTMAIL_EXAMPLE/statusbar.h

SOURCES += $$QTMAIL_EXAMPLE/accountsettings.cpp \
           $$QTMAIL_EXAMPLE/editaccount.cpp \
           $$QTMAIL_EXAMPLE/statusbar.cpp \
           main_messagingaccounts.cpp

RESOURCES += messagingaccounts.qrc

include(../../common.pri)
