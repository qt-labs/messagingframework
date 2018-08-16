TEMPLATE = subdirs
SUBDIRS = qmfclient qmfmessageserver

qmfmessageserver.depends = qmfclient

!contains(DEFINES,QMF_NO_WIDGETS) {
    SUBDIRS += qmfwidgets
    qmfwidgets.depends = qmfclient
}
