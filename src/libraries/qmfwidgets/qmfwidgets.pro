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

TRANSLATIONS += libqmfwidgets-de.ts \
                libqmfwidgets-en_US.ts

RESOURCES += qmfutil.qrc

