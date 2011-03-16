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
          tests \
          examples/qtmail/libs/qmfutil \
          examples/qtmail/app \
          examples/qtmail/plugins/viewers/generic \
          examples/qtmail/plugins/composers/email \
          examples/messagingaccounts \
          examples/serverobserver
          
# disable benchmark test on mac until ported
!macx {
          SUBDIRS += benchmarks
}

symbian {
    message("Building a pure Symbian Client-Server implementation for Data Server using Raptor...")
    system(sbs -b src/symbian/qmfdataserver/bld.inf)
    message("Building a pure Symbian Client-Server implementation for IPC Channel using Raptor...")
    system(sbs -b src/symbian/qmfipcchannelserver/bld.inf)
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
