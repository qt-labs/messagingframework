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

#ifndef QMAILACCOUNT_H
#define QMAILACCOUNT_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>
#include "qmailglobal.h"
#include <QSharedData>
#include "qmailaddress.h"
#include "qmailfolder.h"
#include "qmailmessagefwd.h"
#include "qmailmessagekey.h"
#include "qmailtimestamp.h"

class QMailFolderId;
class QMailAccountPrivate;
class QMailAccountId;

QT_BEGIN_NAMESPACE

class QSettings;
class QTimer;

QT_END_NAMESPACE

class QMF_EXPORT QMailAccount
{
public:
    static const quint64 &SynchronizationEnabled;
    static const quint64 &Synchronized;
    static const quint64 &AppendSignature;
    static const quint64 &UserEditable;
    static const quint64 &UserRemovable;
    static const quint64 &PreferredSender;
    static const quint64 &MessageSource;
    static const quint64 &CanRetrieve;
    static const quint64 &MessageSink;
    static const quint64 &CanTransmit;
    static const quint64 &Enabled;
    static const quint64 &CanReferenceExternalData;
    static const quint64 &CanTransmitViaReference;
    static const quint64 &CanCreateFolders;
    static const quint64 &UseSmartReply;
    static const quint64 &CanSearchOnServer;
    static const quint64 &HasPersistentConnection;

    QMailAccount();
    explicit QMailAccount(const QMailAccountId& id);
    QMailAccount(const QMailAccount& other);

    ~QMailAccount();

    QMailAccount& operator=(const QMailAccount& other);

    QMailAccountId id() const;
    void setId(const QMailAccountId& id);

    QString name() const;
    void setName(const QString &str);

    QMailAddress fromAddress() const;
    void setFromAddress(const QMailAddress &address);

    QString signature() const;
    void setSignature(const QString &str);

    QMailTimeStamp lastSynchronized() const;
    void setLastSynchronized(const QMailTimeStamp &synced);

    QMailMessageMetaDataFwd::MessageType messageType() const;
    void setMessageType(QMailMessageMetaDataFwd::MessageType type);

    QStringList messageSources() const;
    QStringList messageSinks() const;

    QMailFolderId standardFolder(QMailFolder::StandardFolder folder) const;
    void setStandardFolder(QMailFolder::StandardFolder folder, const QMailFolderId &folderId);

    QString iconPath() const;
    void setIconPath(const QString &iconPath);

    const QMap<QMailFolder::StandardFolder, QMailFolderId> &standardFolders() const;

    quint64 status() const;
    void setStatus(quint64 newStatus);
    void setStatus(quint64 mask, bool set);

    static quint64 statusMask(const QString &flagName);

    QString customField(const QString &name) const;
    void setCustomField(const QString &name, const QString &value);
    void setCustomFields(const QMap<QString, QString> &fields);

    void removeCustomField(const QString &name);

    const QMap<QString, QString> &customFields() const;

private:
    friend class QMailAccountPrivate;
    friend class QMailStore;
    friend class QMailStorePrivate;

    void addMessageSource(const QString &source);
    void addMessageSink(const QString &sink);

    bool customFieldsModified() const;
    void setCustomFieldsModified(bool set);

    QSharedDataPointer<QMailAccountPrivate> d;
};

#endif
