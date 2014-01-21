TEMPLATE = subdirs

app.subdir = app
app.target = sub-app

plugins_viewers_generic.subdir = plugins/viewers/generic
plugins_viewers_generic.target = sub-plugins-viewers-generic

plugins_composers_email.subdir = plugins/composers/email
plugins_composers_email.target = sub-plugins-composers-email

SUBDIRS += \
    app \
    plugins_viewers_generic \
    plugins_composers_email

