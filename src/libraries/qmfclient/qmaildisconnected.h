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
