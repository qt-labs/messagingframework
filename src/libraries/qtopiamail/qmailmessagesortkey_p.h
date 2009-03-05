/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGESORTKEY_P_H
#define QMAILMESSAGESORTKEY_P_H

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

#include <QSharedData>
#include <QList>

class QMailMessageSortKeyPrivate : public QSharedData
{
public:
    QMailMessageSortKeyPrivate() : QSharedData() {};

    QList<QMailMessageSortKey::ArgumentType> arguments;
};

#endif

