/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef IMAPMAILBOXPROPERTIES_H
#define IMAPMAILBOXPROPERTIES_H

#include <qmailfolder.h>
#include <qstringlist.h>

struct ImapMailboxProperties
{
    ImapMailboxProperties(const QMailFolder &folder = QMailFolder())
        : id(folder.id()),
          path(folder.path()),
          exists(0),
          recent(0),
          unseen(0),
          uidNext(0)
    {
    }

    bool isSelected() const { return id.isValid(); }

    QMailFolderId id;
    QString path;
    int exists;
    int recent;
    int unseen;
    QString uidValidity;
    int uidNext;
    QString flags;
    QStringList uidList;
};

#endif
