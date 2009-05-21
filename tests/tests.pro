TEMPLATE = subdirs
CONFIG += ordered
CONFIG -= debug_and_release
SUBDIRS = \
      tst_python_email \
      tst_qmailaddress \
      tst_qmailcodec \
      tst_qmailmessage \
      tst_qmailmessagebody \
      tst_qmailmessageheader \
      tst_qmailmessagepart \
      tst_qmailstore \
      tst_qmailstorekeys \
      tst_qprivateimplementation \
      tst_longstring \

CONFIG += unittest

