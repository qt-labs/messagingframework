TEMPLATE = subdirs
SUBDIRS = src/libraries/qtopiamail \
          src/libraries/qtopiamail/tests/tst_longstring \
          src/libraries/qtopiamail/tests/tst_qmailaddress \
          src/libraries/qtopiamail/tests/tst_qmailmessage \
          src/libraries/qtopiamail/tests/tst_qmailmessageheader \
          src/libraries/qtopiamail/tests/tst_qmailstore \
          src/libraries/qtopiamail/tests/tst_python_email \
          src/libraries/qtopiamail/tests/tst_qmailcodec \
          src/libraries/qtopiamail/tests/tst_qmailmessagebody \
          src/libraries/qtopiamail/tests/tst_qmailmessagepart \
          src/libraries/qtopiamail/tests/tst_qprivateimplementation \
          src/libraries/qtopiamail/tests/tst_qmailstorekeys \
          src/libraries/messageserver \
          src/libraries/qmfutil \
          src/plugins/messageservices/imap \
          src/plugins/messageservices/pop \
          src/plugins/messageservices/smtp \
          src/plugins/messageservices/qtopiamailfile \
          src/plugins/contentmanagers/qtopiamailfile \
          src/plugins/viewers/generic \
          src/plugins/composers/email \
          src/settings/messagingaccounts \
          src/tools/messageserver \
          src/tools/messageserver/tests/tst_messageserver \
          src/applications/qtmail
CONFIG += ordered
