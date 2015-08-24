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

#ifndef QMAILACCOUNTKEY_H
#define QMAILACCOUNTKEY_H

#include "qmaildatacomparator.h"
#include "qmailkeyargument.h"
#include "qmailid.h"
#include "qmailmessagefwd.h"
#include <QList>
#include <QSharedData>
#include <QVariant>
#include "qmailipc.h"
#include "qmailglobal.h"

class QMailAccountKeyPrivate;

template <typename Key>
class MailKeyImpl;


class QMF_EXPORT QMailAccountKey
{
public:
    enum Property
    {
        Id = (1 << 0),
        Name = (1 << 1),
        MessageType = (1 << 2),
        FromAddress = (1 << 3),
        Status = (1 << 4),
        Custom = (1 << 5),
        LastSynchronized = (1 << 6),
        IconPath = (1 << 7)
    };

    typedef QMailAccountId IdType;
    typedef QMailKeyArgument<Property> ArgumentType;

    QMailAccountKey();
    QMailAccountKey(const QMailAccountKey& other);
    virtual ~QMailAccountKey();

    QMailAccountKey operator~() const;
    QMailAccountKey operator&(const QMailAccountKey& other) const;
    QMailAccountKey operator|(const QMailAccountKey& other) const;
    const QMailAccountKey& operator&=(const QMailAccountKey& other);
    const QMailAccountKey& operator|=(const QMailAccountKey& other);

    bool operator==(const QMailAccountKey& other) const;
    bool operator !=(const QMailAccountKey& other) const;

    const QMailAccountKey& operator=(const QMailAccountKey& other);

    bool isEmpty() const;
    bool isNonMatching() const;
    bool isNegated() const;

    //for subqueries
    operator QVariant() const;

    const QList<ArgumentType> &arguments() const;
    const QList<QMailAccountKey> &subKeys() const;

    QMailKey::Combiner combiner() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QMailAccountKey nonMatchingKey();

    static QMailAccountKey id(const QMailAccountId &id, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey id(const QMailAccountIdList &ids, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailAccountKey id(const QMailAccountKey &key, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailAccountKey name(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey name(const QString &value, QMailDataComparator::InclusionComparator cmp);
    static QMailAccountKey name(const QStringList &values, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

    static QMailAccountKey messageType(QMailMessageMetaDataFwd::MessageType value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey messageType(int value, QMailDataComparator::InclusionComparator cmp);

    static QMailAccountKey fromAddress(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey fromAddress(const QString &value, QMailDataComparator::InclusionComparator cmp);

    static QMailAccountKey lastSynchronized(const QDateTime &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey lastSynchronized(const QDateTime &value, QMailDataComparator::RelationComparator cmp);

    static QMailAccountKey status(quint64 mask, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailAccountKey status(quint64 mask, QMailDataComparator::EqualityComparator cmp);

    static QMailAccountKey customField(const QString &name, QMailDataComparator::PresenceComparator cmp = QMailDataComparator::Present);
    static QMailAccountKey customField(const QString &name, const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey customField(const QString &name, const QString &value, QMailDataComparator::InclusionComparator cmp);

    static QMailAccountKey iconPath(const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey iconPath(const QString &value, QMailDataComparator::InclusionComparator cmp);
    static QMailAccountKey iconPath(const QStringList &values, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);

private:
    QMailAccountKey(Property p, const QVariant& value, QMailKey::Comparator c);

    template <typename ListType>
    QMailAccountKey(const ListType &valueList, Property p, QMailKey::Comparator c);

    friend class QMailAccountKeyPrivate;
    friend class MailKeyImpl<QMailAccountKey>;

    QSharedDataPointer<QMailAccountKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailAccountKey)

#endif
