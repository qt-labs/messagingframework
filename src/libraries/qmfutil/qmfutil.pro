TEMPLATE = lib 

TARGET = qmfutil 
target.path += $$QMF_INSTALL_ROOT/lib
INSTALLS += target

CONFIG += warn_on

DEPENDPATH += .

INCLUDEPATH += . ../qtopiamail ../qtopiamail/support

LIBS += -L../qtopiamail -lqtopiamail

HEADERS += qmailcomposer.h \
           qmailviewer.h

SOURCES += qmailcomposer.cpp \
           qmailviewer.cpp

TRANSLATIONS += libqmfutil-ar.ts \
                libqmfutil-de.ts \
                libqmfutil-en_GB.ts \
                libqmfutil-en_SU.ts \
                libqmfutil-en_US.ts \
                libqmfutil-es.ts \
                libqmfutil-fr.ts \
                libqmfutil-it.ts \
                libqmfutil-ja.ts \
                libqmfutil-ko.ts \
                libqmfutil-pt_BR.ts \
                libqmfutil-zh_CN.ts \
                libqmfutil-zh_TW.ts
