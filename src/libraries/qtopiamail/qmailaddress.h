/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/
#ifndef QMAILADDRESS_H
#define QMAILADDRESS_H

#include "qmailglobal.h"
#ifdef QMAIL_QTOPIA
#include <QContact>
#endif
#include "qmailipc.h"
#include <QList>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>

class QMailAddressPrivate;

class QTOPIAMAIL_EXPORT QMailAddress
{
public:
    QMailAddress();
    explicit QMailAddress(const QString& addressText);
    QMailAddress(const QString& name, const QString& emailAddress);
    QMailAddress(const QMailAddress& other);
    ~QMailAddress();

    bool isNull() const;

    QString name() const;
    QString address() const;

    bool isGroup() const;
    QList<QMailAddress> groupMembers() const;
#ifdef QMAIL_QTOPIA
    QString displayName() const;
    QString displayName(QContactModel& fromModel) const;
    QContact matchContact() const;
    bool matchesExistingContact() const;
    QContact matchContact(QContactModel& fromModel) const;
    bool isChatAddress() const;
    QString chatIdentifier() const;
#endif

    bool isPhoneNumber() const;
    bool isEmailAddress() const;

    QString minimalPhoneNumber() const;

    QString toString(bool forceDelimited = false) const;

    bool operator==(const QMailAddress& other) const;
    bool operator!=(const QMailAddress& other) const;

    const QMailAddress& operator=(const QMailAddress& other);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QStringList toStringList(const QList<QMailAddress>& list, bool forceDelimited = false);
    static QList<QMailAddress> fromStringList(const QString& list);
    static QList<QMailAddress> fromStringList(const QStringList& list);

    static QString removeComments(const QString& input);
    static QString removeWhitespace(const QString& input);

    static QString phoneNumberPattern();
    static QString emailAddressPattern();

private:
    QSharedDataPointer<QMailAddressPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailAddress)

typedef QList<QMailAddress> QMailAddressList;

Q_DECLARE_METATYPE(QMailAddressList);
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailAddressList, QMailAddressList);

#endif
