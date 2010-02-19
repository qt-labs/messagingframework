macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET=10.5
}

target.path+=$$QMF_INSTALL_ROOT/tests

QTOPIAMAIL=../../src/libraries/qtopiamail
DEPENDPATH += .
INCLUDEPATH += . $$QTOPIAMAIL $$QTOPIAMAIL/support
LIBS += -L$$QTOPIAMAIL/build
QMAKE_LFLAGS += -Wl,-rpath,$$QTOPIAMAIL

include(../common.pri)

