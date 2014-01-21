TEMPLATE = app
TARGET = qtmail5

# TODO: example linking against private headers is bad
QT += widgets qmfclient qmfclient-private qmfmessageserver qmfwidgets

target.path += $$QMF_INSTALL_ROOT/bin

HEADERS += emailclient.h \
           messagelistview.h \
           searchview.h \
           selectcomposerwidget.h \
           readmail.h \
           writemail.h \
           accountsettings.h \
           editaccount.h \
           statusmonitorwidget.h \
           statusbar.h \
           statusmonitor.h

SOURCES += emailclient.cpp \
           main.cpp \
           messagelistview.cpp \
           searchview.cpp \
           selectcomposerwidget.cpp \
           readmail.cpp \
           writemail.cpp \
           accountsettings.cpp \
           editaccount.cpp \
           statusmonitorwidget.cpp \
           statusbar.cpp \
           statusmonitor.cpp

FORMS += searchviewbasephone.ui

RESOURCES += qtmail.qrc

include(../../../common.pri)
