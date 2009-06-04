TEMPLATE = app
CONFIG -= debug_and_release

TARGET = qtmail 
target.path += $$QMF_INSTALL_ROOT/bin
INSTALLS += target

DEPENDPATH += . 

INCLUDEPATH += . ../../libraries/qtopiamail \
                 ../../libraries/qtopiamail/support \
                 ../../libraries/qmfutil

LIBS += -L../../libraries/qtopiamail -lqtopiamail \
        -L../../libraries/qmfutil -lqmfutil

HEADERS += foldermodel.h \
           folderdelegate.h \
           folderview.h \
           emailfoldermodel.h \
           emailfolderview.h \
           messagelistview.h \
           messagestore.h \
           messagefolder.h \
           emailclient.h \
           selectfolder.h \
           statusdisplay_p.h \
           searchview.h \
           readmail.h \
           writemail.h \
           selectcomposerwidget.h

SOURCES += main.cpp \
           foldermodel.cpp \
           folderdelegate.cpp \
           folderview.cpp \
           emailfoldermodel.cpp \
           emailfolderview.cpp \
           messagelistview.cpp \
           messagestore.cpp \
           messagefolder.cpp \
           emailclient.cpp \
           selectfolder.cpp \
           statusdisplay.cpp \
           searchview.cpp \
           readmail.cpp \
           writemail.cpp \
           selectcomposerwidget.cpp

FORMS += searchviewbasephone.ui

RESOURCES += qtmail.qrc

include(../../common.pri)

