/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILACCOUNTKEY_P_H
#define QMAILACCOUNTKEY_P_H

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

#include "qmailaccountkey.h"
#include "mailkeyimpl_p.h"


class QMailAccountKeyPrivate : public MailKeyImpl<QMailAccountKey>
{
public:
    typedef MailKeyImpl<QMailAccountKey> Impl;

    QMailAccountKeyPrivate() : Impl() {}
    QMailAccountKeyPrivate(QMailAccountKey::Property p, const QVariant &v, QMailKey::Comparator c) : Impl(p, v, c) {}

    template <typename ListType>
    QMailAccountKeyPrivate(const ListType &ids, QMailAccountKey::Property p, QMailKey::Comparator c) : Impl(ids, p, c) {}
};


#endif

