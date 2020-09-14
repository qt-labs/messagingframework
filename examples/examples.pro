TEMPLATE = subdirs

!contains(DEFINES,QMF_NO_WIDGETS) {
    SUBDIRS = serverobserver
    # To be migrated...
    lessThan(QT_MAJOR_VERSION, 6) {
        SUBDIRS += qtmail messagingaccounts
    }
}
