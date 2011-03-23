TEMPLATE = subdirs

load(data_caging_paths)

SUBDIRS =
TARGET = "QMF"

VERSION = 1.0.0

vendorinfo = \
    "; Localised Vendor name" \
    "%{\"Nokia\"}" \
    " " \
    "; Unique Vendor name" \
    ":\"Nokia\"" \
    " "

qmfdeployment.pkg_prerules += vendorinfo

deploy.path = C:

qmfdeployment.sources = messageserver.exe
qmfdeployment.sources += qmfdataserver.exe
qmfdeployment.sources += qmfipcchannelserver.exe
qmfdeployment.sources += qmfclient.dll
qmfdeployment.sources += qmfmessageserver.dll
qmfdeployment.path = /sys/bin
DEPLOYMENT += qmfdeployment

qmfcontentmanagerpluginstub.sources = qmfstoragemanager.dll
qmfcontentmanagerpluginstub.path = $$QT_PLUGINS_BASE_DIR/qtmail/contentmanagers
DEPLOYMENT += qmfcontentmanagerpluginstub

qmfmessageservicepluginstubs.sources = imap.dll
qmfmessageservicepluginstubs.sources += pop.dll
qmfmessageservicepluginstubs.sources += smtp.dll
qmfmessageservicepluginstubs.sources += qmfsettings.dll
qmfmessageservicepluginstubs.path = $$QT_PLUGINS_BASE_DIR/qtmail/messageservices
DEPLOYMENT += qmfmessageservicepluginstubs

qmfconfigfile.sources = qmfconfig.ini
qmfconfigfile.path = c:/data/
DEPLOYMENT += qmfconfigfile
