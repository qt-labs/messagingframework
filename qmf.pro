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
CONFIG -= debug_and_release

# Make it so projects can find our specific features
#system(touch $$OUT_PWD/.qmake.cache)
#system(if ! [ -e $$OUT_PWD/features ]; then ln -s $$PWD/features $$OUT_PWD/features; fi)

