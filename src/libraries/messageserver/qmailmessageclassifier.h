/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGECLASSIFIER_H
#define MESSAGECLASSIFIER_H

#include <qmailglobal.h>
#include <QStringList>

class QMailMessageMetaData;
class QMailMessage;

class QTOPIAMAIL_EXPORT QMailMessageClassifier
{
public:
    QMailMessageClassifier();
    ~QMailMessageClassifier();

    bool classifyMessage(QMailMessageMetaData& message);
    bool classifyMessage(QMailMessage& message);

private:
    QStringList voiceMailAddresses;
    QStringList videoMailAddresses;
};

#endif
