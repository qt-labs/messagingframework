TARGET     = QmfWidgets
QT         = core qmfclient widgets
CONFIG    += warn_on

load(qt_module)
CONFIG -= create_cmake

DEFINES -= QT_NO_CAST_TO_ASCII # this is a fight for another day.

DEFINES += QMFUTIL_INTERNAL

HEADERS += emailfoldermodel.h \
           emailfolderview.h \
           folderdelegate.h \
           foldermodel.h \
           folderview.h \
           selectfolder.h \
           qtmailnamespace.h

SOURCES += emailfoldermodel.cpp \
           emailfolderview.cpp \
           folderdelegate.cpp \
           foldermodel.cpp \
           folderview.cpp \
           selectfolder.cpp \
           qtmailnamespace.cpp

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

RESOURCES += qmfutil.qrc

