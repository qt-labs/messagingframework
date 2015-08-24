/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef IMAPMAILBOXPROPERTIES_H
#define IMAPMAILBOXPROPERTIES_H

#include <qmailfolder.h>
#include <qstringlist.h>
#include "integerregion.h"

typedef uint MessageFlags;
typedef QPair<QString, MessageFlags> FlagChange;

struct ImapMailboxProperties
{
    ImapMailboxProperties(const QMailFolder &folder = QMailFolder())
        : id(folder.id()),
          path(folder.path()),
          status(folder.status()),
          exists(0),
          recent(0),
          unseen(0),
          uidNext(0),
          searchCount(0),
          noModSeq(true)
    {
    }

    bool isSelected() const { return id.isValid(); }

    QMailFolderId id;
    QString path;
    quint64 status;
    quint32 exists;
    quint32 recent;
    quint32 unseen;
    QString uidValidity;
    quint32 uidNext;
    QString flags;
    QStringList uidList;
    uint searchCount;
    QList<uint> msnList;
    QString highestModSeq;
    bool noModSeq;
    QStringList permanentFlags;
    QString vanished;
    QList<FlagChange> flagChanges;
};

#endif
