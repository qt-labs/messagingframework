TEMPLATE = subdirs
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
      tst_storagemanager \
      tst_qmaildisconnected \
      tst_qmailnamespace \
      tst_locks \
      tst_qmailthread

exists(/usr/bin/gpgme-config) {
    SUBDIRS += tst_crypto
}

# these tests fail to build/pass on windows.
# longer term, we should remove this stuff entirely in favour of Qt's
# categorised logging.
!win32 {
    SUBDIRS += \
        tst_qmaillog \
        tst_qcop \
        tst_qlogsystem
}

CONFIG += unittest

# Install test file description
test_description.files = qt5/tests.xml
test_description.path  = $$QMF_INSTALL_ROOT/tests5
INSTALLS += test_description
