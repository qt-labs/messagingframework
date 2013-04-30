TEMPLATE = subdirs
CONFIG += ordered
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
      tst_longstream \
      tst_qmailmessageset \
      tst_qmailserviceaction \
      tst_qmailstorageaction \
      tst_qmail_sortkeys \
      tst_qmail_listmodels \
      tst_qmaillog \
      tst_qcop \
      tst_storagemanager \
      tst_qmaildisconnected \
      tst_qmailnamespace \
      tst_qlogsystem \
      tst_locks \
      tst_qmailthread \


CONFIG += unittest

# Install test file description
equals(QT_MAJOR_VERSION, 4) {
    test_description.files = tests.xml
    test_description.path  = $$QMF_INSTALL_ROOT/tests
}
equals(QT_MAJOR_VERSION, 5) {
    test_description.files = qt5/tests.xml
    test_description.path  = $$QMF_INSTALL_ROOT/tests5
}
INSTALLS += test_description
