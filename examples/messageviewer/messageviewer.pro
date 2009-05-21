TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
CONFIG -= debug_and_release

# Input
HEADERS += messagedelegate.h messagemodel.h messageviewer.h
FORMS += messageviewerbase.ui
SOURCES += main.cpp messagedelegate.cpp messagemodel.cpp messageviewer.cpp
