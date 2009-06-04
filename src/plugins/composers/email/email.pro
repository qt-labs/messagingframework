TEMPLATE = lib 

TARGET = emailcomposer 
target.path += $$QMF_INSTALL_ROOT/plugins/composers
INSTALLS += target

DEPENDPATH += .

INCLUDEPATH += . ../../../libraries/qmfutil \
               ../../../libraries/qtopiamail \
               ../../../libraries/qtopiamail/support

LIBS += -L../../../libraries/qtopiamail -lqtopiamail \
        -L../../../libraries/qmfutil -lqmfutil

HEADERS += emailcomposer.h \
           attachmentlistwidget.h

SOURCES += emailcomposer.cpp \
           attachmentlistwidget.cpp

TRANSLATIONS += libemailcomposer-ar.ts \
                libemailcomposer-de.ts \
                libemailcomposer-en_GB.ts \
                libemailcomposer-en_SU.ts \
                libemailcomposer-en_US.ts \
                libemailcomposer-es.ts \
                libemailcomposer-fr.ts \
                libemailcomposer-it.ts \
                libemailcomposer-ja.ts \
                libemailcomposer-ko.ts \
                libemailcomposer-pt_BR.ts \
                libemailcomposer-zh_CN.ts \
                libemailcomposer-zh_TW.ts

RESOURCES += email.qrc                

include(../../../common.pri)

