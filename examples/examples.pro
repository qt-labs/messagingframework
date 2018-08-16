TEMPLATE = subdirs

!contains(DEFINES,QMF_NO_WIDGETS) {
    SUBDIRS = qtmail messagingaccounts serverobserver
}
