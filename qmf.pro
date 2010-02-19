TEMPLATE = subdirs
SUBDIRS = src/libraries/qtopiamail \
          src/libraries/messageserver \
          src/libraries/qmfutil \
          src/plugins/messageservices/imap \
          src/plugins/messageservices/pop \
          src/plugins/messageservices/smtp \
          src/plugins/messageservices/qtopiamailfile \
          src/plugins/contentmanagers/qtopiamailfile \
          src/plugins/viewers/generic \
          src/plugins/composers/email \
          src/tools/messageserver \
          examples/applications/qtmail \
          examples/settings/messagingaccounts \
          tests \

# disbale benchmark test on mac until ported
!macx {
          SUBDIRS += benchmarks
}

CONFIG += ordered

# Custom target 'doc' to generate documentation
dox.target = doc
dox.commands = THISYEAR=2009 qdoc3 $$_PRO_FILE_PWD_/doc/src/qmf.qdocconf
dox.depends =

QMAKE_EXTRA_TARGETS += dox

