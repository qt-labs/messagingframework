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
      tst_qmailaccountconfiguration \
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
      tst_qmailthread \
      tst_pop \
      tst_imap \
      tst_smtp

exists(/usr/bin/gpgme-config) {
    SUBDIRS += tst_crypto
}

CONFIG += unittest

# Install test file description
test_description.files = qt5/tests.xml
test_description.path  = $$QMF_INSTALL_ROOT/tests5
INSTALLS += test_description
