/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
