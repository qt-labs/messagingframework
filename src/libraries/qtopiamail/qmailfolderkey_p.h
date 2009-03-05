/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILFOLDERKEY_P_H
#define QMAILFOLDERKEY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmailfolderkey.h"
#include "mailkeyimpl_p.h"


class QMailFolderKeyPrivate : public MailKeyImpl<QMailFolderKey>
{
public:
    typedef MailKeyImpl<QMailFolderKey> Impl;

    QMailFolderKeyPrivate() : Impl() {}
    QMailFolderKeyPrivate(QMailFolderKey::Property p, const QVariant &v, QMailKey::Comparator c) : Impl(p, v, c) {}

    template <typename ListType>
    QMailFolderKeyPrivate(const ListType &valueList, QMailFolderKey::Property p, QMailKey::Comparator c) : Impl(valueList, p, c) {}
};


#endif

