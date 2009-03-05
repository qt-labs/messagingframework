/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include <newcountnotifier.h>

//stub for alternate handling, currently does nothing

NewCountNotifier::NewCountNotifier(QMailMessage::MessageType, int)
:
    QObject()
{
}

NewCountNotifier::~NewCountNotifier()
{
}

bool NewCountNotifier::notify()
{
    //ensure a response is returned
    emit response(true);
    return true;
}

void NewCountNotifier::notify(QMailMessage::MessageType, int)
{
}

