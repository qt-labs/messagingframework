TEMPLATE = subdirs
SUBDIRS = qmfclient qmfmessageserver qmfwidgets

qmfmessageserver.depends = qmfclient
qmfwidgets.depends = qmfclient
