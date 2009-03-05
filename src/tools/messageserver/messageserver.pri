QTOPIA*=mail
MODULES*=messageserver

enable_cell:contains(PROJECTS,libraries/qtopiasmil):CONFIG+=enable_mms
else:DEFINES+=QTOPIA_NO_SMS QTOPIA_NO_MMS

!enable_telephony:DEFINES+=QTOPIA_NO_COLLECTIVE

HEADERS=\
    servicehandler.h\
    mailmessageclient.h\
    messageserver.h\
    newcountnotifier.h

SOURCES=\
    servicehandler.cpp\
    mailmessageclient.cpp\
    messageserver.cpp\
    prepareaccounts.cpp\
    newcountnotifier.cpp\
    main.cpp

