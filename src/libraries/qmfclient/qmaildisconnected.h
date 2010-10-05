/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILDISCONNECTED_H
#define QMAILDISCONNECTED_H

#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailfolder.h"

class QMF_EXPORT QMailDisconnected
{
public:
    static QMailMessageKey destinationKey(const QMailFolderId &folderId);
    static QMailMessageKey sourceKey(const QMailFolderId &folderId);
    static QMailFolderId sourceFolderId(const QMailMessageMetaData &metaData);
    static QMailMessageKey::Properties parentFolderProperties();
    static void clearPreviousFolder(QMailMessageMetaData *message);
    static void copyPreviousFolder(const QMailMessageMetaData &source, QMailMessageMetaData *dest);
    static QMap<QMailFolderId, QMailMessageIdList> restoreMap(const QMailMessageIdList &messageIds);
    static void rollBackUpdates(const QMailAccountId &mailAccountId);
    static bool updatesOutstanding(const QMailAccountId &mailAccountId);
    static void moveToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder);
    static void moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId);
    static void moveToFolder(QMailMessageMetaData *message, const QMailFolderId &folderId);
    static void copyToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder);
    static void copyToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId);
    static void flagMessages(const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask, const QString& description);
    static void flagMessage(const QMailMessageId& id, quint64 setMask, quint64 unsetMask, const QString& description);

    static void restoreToPreviousFolder(const QMailMessageId& id);
    static void restoreToPreviousFolder(const QMailMessageKey& key);

};
#endif
