TEMPLATE = subdirs
SUBDIRS = src tests

tests.depends = src

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {
    SUBDIRS += examples
    examples.depends = src

    # disable benchmark test on mac until ported
    !macx {
        !SERVER_AS_DLL {
            SUBDIRS += benchmarks
            benchmarks.depends = src
        }
    }
}

defineReplace(targetPath) {
    return($$replace(1, /, $$QMAKE_DIR_SEP))
}

# Custom target 'doc' to generate documentation
dox.target = doc
dox.commands = qdoc3 $$_PRO_FILE_PWD_/doc/src/qmf.qdocconf
dox.depends =

QMAKE_EXTRA_TARGETS += dox

include(doc/src/doc.pri)

!unix {
     warning("IMAP COMPRESS capability is currently not supported on non unix platforms")
}
