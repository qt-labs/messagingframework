/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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


class QTOPIAMAIL_EXPORT QMailAccountKey
{
public:
    enum Property
    {
        Id = (1 << 0),
        Name = (1 << 1),
        MessageType = (1 << 2),
        FromAddress = (1 << 3),
        Status = (1 << 4),
        Custom = (1 << 5)
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

    static QMailAccountKey status(quint64 mask, QMailDataComparator::InclusionComparator cmp = QMailDataComparator::Includes);
    static QMailAccountKey status(quint64 mask, QMailDataComparator::EqualityComparator cmp);

    static QMailAccountKey customField(const QString &name, QMailDataComparator::PresenceComparator cmp = QMailDataComparator::Present);
    static QMailAccountKey customField(const QString &name, const QString &value, QMailDataComparator::EqualityComparator cmp = QMailDataComparator::Equal);
    static QMailAccountKey customField(const QString &name, const QString &value, QMailDataComparator::InclusionComparator cmp);

private:
    QMailAccountKey(Property p, const QVariant& value, QMailKey::Comparator c);

    template <typename ListType>
    QMailAccountKey(const ListType &valueList, Property p, QMailKey::Comparator c);

    friend class QMailAccountKeyPrivate;
    friend class MailKeyImpl<QMailAccountKey>;

    QSharedDataPointer<QMailAccountKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailAccountKey);

#endif
