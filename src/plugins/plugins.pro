TEMPLATE = subdirs
SUBDIRS = \
    messageservices/imap \
    messageservices/pop \
    messageservices/smtp \
    contentmanagers/qmfstoragemanager

exists(/usr/bin/gpgme-config) {
    SUBDIRS += crypto/gpgme
    SUBDIRS += crypto/smime
}

packagesExist(libsignon-qt5) {
    SUBDIRS += credentials/sso
}
