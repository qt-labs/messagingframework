TEMPLATE = app
TARGET = qtmail5
target.path += $$[QT_INSTALL_EXAMPLES]/qmf/qtmail
QT += widgets qmfclient qmfmessageserver qmfwidgets

# Use webkit to render mail if available
contains(QT_CONFIG,webkit){
    QT += network webkitwidgets
    DEFINES += USE_WEBKIT
}

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
           statusmonitor.h \
           qmailcomposer.h \
           qmailviewer.h \
           attachmentlistwidget.h \
           detailspage_p.h \
           emailcomposer.h \
           attachmentoptions.h \
           browserwidget.h \
           genericviewer.h

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
           statusmonitor.cpp \
           qmailcomposer.cpp \
           qmailviewer.cpp \
           attachmentlistwidget.cpp \
           detailspage.cpp \
           emailcomposer.cpp \
           attachmentoptions.cpp \
           browserwidget.cpp \
           genericviewer.cpp

FORMS += searchviewbasephone.ui

RESOURCES += qtmail.qrc

include(../../common.pri)
CONFIG += install_ok
