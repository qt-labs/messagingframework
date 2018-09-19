TEMPLATE = subdirs
SUBDIRS = \
    messageservices/imap \
    messageservices/pop \
    messageservices/smtp \
    messageservices/qmfsettings \
    contentmanagers/qmfstoragemanager

exists(/usr/bin/gpgme-config) {
    SUBDIRS += crypto/gpgme
    SUBDIRS += crypto/smime
}

