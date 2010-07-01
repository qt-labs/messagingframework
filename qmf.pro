TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = src/libraries/qtopiamail \
          src/libraries/messageserver \
          src/plugins/messageservices/imap \
          src/plugins/messageservices/pop \
          src/plugins/messageservices/smtp \
          src/plugins/messageservices/qtopiamailfile \
          src/plugins/contentmanagers/qtopiamailfile \
          src/tools/messageserver \
          tests \
          examples/qtmail/libs/qmfutil \
          examples/qtmail/app \
          examples/qtmail/plugins/viewers/generic \
          examples/qtmail/plugins/composers/email \
          examples/messagingaccounts \
          
# disable benchmark test on mac until ported
!macx {
          SUBDIRS += benchmarks
}

defineReplace(targetPath) {
    return($$replace(1, /, $$QMAKE_DIR_SEP))
}

include(doc/src/doc.pri)

