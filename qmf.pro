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
          examples/settings/messagingaccounts \
          
# disable benchmark test on mac until ported
!macx {
          SUBDIRS += benchmarks
}


# Custom target 'doc' to generate documentation
dox.target = doc
dox.commands = qdoc3 $$_PRO_FILE_PWD_/doc/src/qmf.qdocconf
dox.depends =

QMAKE_EXTRA_TARGETS += dox

