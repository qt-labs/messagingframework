TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = src/libraries/qmfclient \
          src/libraries/qmfmessageserver \
          src/plugins/messageservices/imap \
          src/plugins/messageservices/pop \
          src/plugins/messageservices/smtp \
          src/plugins/messageservices/qmfsettings \
          src/plugins/contentmanagers/qmfstoragemanager \
          src/tools/messageserver \
          tests

!contains(DEFINES,QMF_NO_MESSAGE_SERVICE_EDITOR) {

SUBDIRS += \
          examples/qtmail/libs/qmfutil \
          examples/qtmail/app \
          examples/qtmail/plugins/viewers/generic \
          examples/qtmail/plugins/composers/email \
          examples/messagingaccounts \
          examples/serverobserver
          
# disable benchmark test on mac until ported
!macx {
    !SERVER_AS_DLL {
          SUBDIRS += benchmarks
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
