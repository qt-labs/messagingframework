LIBS += $$system(gpgme-config --libs)

INCLUDEPATH += $$PWD/common/

HEADERS += $$PWD/common/qgpgme.h
SOURCES += $$PWD/common/qgpgme.cpp
