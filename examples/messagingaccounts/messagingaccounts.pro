TEMPLATE = app
target.path += $$QMF_INSTALL_ROOT/bin
TARGET = messagingaccounts5
QT += widgets qmfclient qmfmessageserver qmfwidgets

QTMAIL_EXAMPLE=../qtmail

INCLUDEPATH += \
                 $$QTMAIL_EXAMPLE/app

HEADERS += $$QTMAIL_EXAMPLE/app/accountsettings.h \
           $$QTMAIL_EXAMPLE/app/editaccount.h \
           $$QTMAIL_EXAMPLE/app/statusbar.h

SOURCES += $$QTMAIL_EXAMPLE/app/accountsettings.cpp \
           $$QTMAIL_EXAMPLE/app/editaccount.cpp \
           $$QTMAIL_EXAMPLE/app/statusbar.cpp \
           main_messagingaccounts.cpp

RESOURCES += messagingaccounts.qrc

include(../../common.pri)
