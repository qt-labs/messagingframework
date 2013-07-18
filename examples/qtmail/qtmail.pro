TEMPLATE = subdirs

libs_qmfutil.subdir = libs/qmfutil
libs_qmfutil.target = sub-qmfutil

app.subdir = app
app.target = sub-app
app.depends = sub-qmfutil

plugins_viewers_generic.subdir = plugins/viewers/generic
plugins_viewers_generic.target = sub-plugins-viewers-generic
plugins_viewers_generic.depends = sub-qmfutil

plugins_composers_email.subdir = plugins/composers/email
plugins_composers_email.target = sub-plugins-composers-email
plugins_composers_email.depends = sub-qmfutil

SUBDIRS += \
    libs_qmfutil \
    app \
    plugins_viewers_generic \
    plugins_composers_email

