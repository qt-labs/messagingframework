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

#ifndef QMAILMESSAGEKEY_H
#define QMAILMESSAGEKEY_H

#include "qmaildatacomparator.h"
#include "qmailkeyargument.h"
#include "qmailid.h"
#include "qmailmessagefwd.h"
#include <QFlags>
#include <QList>
#include <QSharedData>
#include <QVariant>
#include "qmailglobal.h"
#include "qmailipc.h"

class QMailAccountKey;
class QMailFolderKey;
class QMailThreadKey;

class QMailMessageKeyPrivate;

template <typename Key>
class MailKeyImpl;


class QMF_EXPORT QMailMessageKey
{
public:
    enum Property
    {
        Id = (1 << 0),
        Type = (1 << 1),
        ParentFolderId = (1 << 2),
        Sender = (1 << 3),
        Recipients = (1 << 4),
        Subject = (1 << 5),
        TimeStamp = (1 << 6),
        Status = (1 << 7),
        Conversation = (1 << 8),
        ReceptionTimeStamp = (1 << 9),
        ServerUid = (1 << 10),
        Size = (1 << 11),
        ParentAccountId = (1 << 12),
        AncestorFolderIds = (1 << 13),
        ContentType = (1 << 14),
        PreviousParentFolderId = (1 << 15),
        ContentScheme = (1 << 16),
        ContentIdentifier = (1 << 17),
        InResponseTo = (1 << 18),
        ResponseType = (1 << 19),
        Custom = (1 << 20),
        CopyServerUid = (1 << 21),
        RestoreFolderId = (1 << 22),
        ListId = (1 << 23),
        RfcId = (1 << 24),
        Preview = (1 << 25),
        ParentThreadId = (1 << 26)
    };
    Q_DECLARE_FLAGS(Properties,Property)

    typedef QMailMessageId IdType;
    typedef QMailKeyArgument<Property> ArgumentType;

    QMailMessageKey();
    QMailMessageKey(const QMailMessageKey& other);
    virtual ~QMailMessageKey();

    QMailMessageKey operator~() const;
    QMailMessageKey operator&(const QMailMessageKey& other) const;
    QMailMessageKey operator|(const QMailMessageKey& other) const;
    const QMailMessageKey& operator&=(const QMailMessageKey& other);
    const QMailMessageKey& operator|=(const QMailMessageKey& other);

    bool operator==(const QMailMessageKey& other) const;
    bool operator !=(const QMailMessageKey& other) const;

    const QMailMessageKey& operator=(const QMailMessageKey& other);

    bool isEmpty() const;
    bool isNonMatching() const;
    bool isNegated() const;

    operator QVariant() const;

    const QList<ArgumentType> &arguments() const;
    const QList<QMailMessageKey> &subKeys() const;

    QMailKey::Combiner combiner() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    void serialize(QDataStream& stream) const;
    void deserialize(QDataStream& stream);
    
    static QMailMessageKey nonMatchingKey();

    static QMailMessageKey id(const QMailMessageId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey id(const QMailMessageIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey id(const QMailMessageKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey messageType(QMailMessageMetaDataFwd::MessageType type, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey messageType(int type, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey parentFolderId(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey parentFolderId(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey parentFolderId(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey sender(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey sender(const QString &value, QMailDataComparator::InclusionComparator cmp);
    static QMailMessageKey sender(const QString &value, QMailDataComparator::RelationComparator cmp);
    static QMailMessageKey sender(const QStringList &values, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey recipients(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey recipients(const QString &value, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey subject(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey subject(const QString &value, QMailDataComparator::InclusionComparator cmp);
    static QMailMessageKey subject(const QStringList &values, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey preview(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey preview(const QString &value, QMailDataComparator::InclusionComparator cmp);
    static QMailMessageKey preview(const QStringList &values, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey timeStamp(const QDateTime &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey timeStamp(const QDateTime &value, QMailDataComparator::RelationComparator cmp);

    static QMailMessageKey receptionTimeStamp(const QDateTime &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey receptionTimeStamp(const QDateTime &value, QMailDataComparator::RelationComparator cmp);

    static QMailMessageKey status(quint64 mask, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey status(quint64 mask, QMailDataComparator::EqualityComparator cmp);

    static QMailMessageKey serverUid(const QString &uid, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey serverUid(const QString &uid, QMailDataComparator::InclusionComparator cmp);
    static QMailMessageKey serverUid(const QStringList &uids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey size(int value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey size(int value, QMailDataComparator::RelationComparator cmp);

    static QMailMessageKey parentAccountId(const QMailAccountId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey parentAccountId(const QMailAccountIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey parentAccountId(const QMailAccountKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey ancestorFolderIds(const QMailFolderId &id, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey ancestorFolderIds(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey ancestorFolderIds(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey contentType(QMailMessageMetaDataFwd::ContentType type, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey contentType(const QList<QMailMessageMetaDataFwd::ContentType> &types, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey previousParentFolderId(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey previousParentFolderId(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey previousParentFolderId(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey contentScheme(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey contentScheme(const QString &value, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey contentIdentifier(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey contentIdentifier(const QString &value, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey inResponseTo(const QMailMessageId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey inResponseTo(const QMailMessageIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey inResponseTo(const QMailMessageKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey responseType(QMailMessageMetaDataFwd::ResponseType type, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey responseType(const QList<QMailMessageMetaDataFwd::ResponseType> &types, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey customField(const QString &name, QMailDataComparator::PresenceComparator cmp = QMailDataComparator::Present);
    static QMailMessageKey customField(const QString &name, const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey customField(const QString &name, const QString &value, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey conversation(const QMailMessageId &id);
    static QMailMessageKey conversation(const QMailMessageIdList &ids);
    static QMailMessageKey conversation(const QMailMessageKey &key);

    static QMailMessageKey copyServerUid(const QString &uid, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey copyServerUid(const QString &uid, QMailDataComparator::InclusionComparator cmp);
    static QMailMessageKey copyServerUid(const QStringList &uids, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey restoreFolderId(const QMailFolderId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey restoreFolderId(const QMailFolderIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey restoreFolderId(const QMailFolderKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailMessageKey listId(const QString &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey listId(const QString &id, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey rfcId(const QString &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey rfcId(const QString &id, QMailDataComparator::InclusionComparator cmp);

    static QMailMessageKey parentThreadId(const QMailThreadId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailMessageKey parentThreadId(const QMailThreadIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailMessageKey parentThreadId(const QMailThreadKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

private:
    QMailMessageKey(Property p, const QVariant& value, QMailKey::Comparator c);

    template <typename ListType>
    QMailMessageKey(const ListType &valueList, Property p, QMailKey::Comparator c);

    friend class QMailMessageKeyPrivate;
    friend class MailKeyImpl<QMailMessageKey>;

    QSharedDataPointer<QMailMessageKeyPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMailMessageKey::Properties)
Q_DECLARE_USER_METATYPE(QMailMessageKey)

#endif
