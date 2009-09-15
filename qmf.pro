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
          benchmarks \

CONFIG += ordered

