/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGEMODELBASE_H
#define QMAILMESSAGEMODELBASE_H

#include <QAbstractItemModel>
#include "qmailglobal.h"

class QMailMessageMetaData;

class QTOPIAMAIL_EXPORT QMailMessageModelBase
{
public:
    enum Roles 
    {
        MessageAddressTextRole = Qt::UserRole, //sender or recipient address
        MessageSubjectTextRole, //subject
        MessageFilterTextRole, //the address text role concatenated with subject text role
        MessageTimeStampTextRole, //timestamp text
        MessageTypeIconRole, //mms,email,sms icon
        MessageStatusIconRole, //read,unread,downloaded icon 
        MessageDirectionIconRole, //incoming or outgoing message icon
        MessagePresenceIconRole, 
        MessageBodyTextRole, //body text, only if textual data
        MessageIdRole
    };

protected:
    QVariant data(const QMailMessageMetaData &message, int role) const;
};

#endif
