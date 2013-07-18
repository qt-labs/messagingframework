TEMPLATE = subdirs
SUBDIRS = qmfclient qmfmessageserver

qmfmessageserver.depends = qmfclient
