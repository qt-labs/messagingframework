TEMPLATE = app
TARGET = qtmail5
QT += widgets qmfclient qmfclient-private qmfmessageserver # TODO: example linking against private headers is bad

macx:contains(QT_CONFIG, qt_framework) {
    LIBS += -framework qmfutil5
} else {
    LIBS += -lqmfutil5
}

target.path += $$QMF_INSTALL_ROOT/bin

INCLUDEPATH += \
                 ../libs/qmfutil \

LIBS += \
        -L../libs/qmfutil/build

macx:LIBS += \
             -F../libs/qmfutil/build

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
