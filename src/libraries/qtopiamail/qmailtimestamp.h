/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/
#ifndef QMAILTIMESTAMP_H
#define QMAILTIMESTAMP_H

#include "qmailglobal.h"

#include <QDateTime>
#include <QSharedDataPointer>
#include <QString>

class QMailTimeStampPrivate;

class QTOPIAMAIL_EXPORT QMailTimeStamp
{
public:
    static QMailTimeStamp currentDateTime();

    QMailTimeStamp();
    explicit QMailTimeStamp(const QString& timeText);
    explicit QMailTimeStamp(const QDateTime& dateTime);
    QMailTimeStamp(const QMailTimeStamp& other);
    ~QMailTimeStamp();

    QString toString() const;

    QDateTime toLocalTime() const;
    QDateTime toUTC() const;

    bool isNull() const;
    bool isValid() const;

    bool operator== (const QMailTimeStamp& other) const;
    bool operator!= (const QMailTimeStamp& other) const;

    bool operator< (const QMailTimeStamp& other) const;
    bool operator<= (const QMailTimeStamp& other) const;

    bool operator> (const QMailTimeStamp& other) const;
    bool operator>= (const QMailTimeStamp& other) const;

    const QMailTimeStamp& operator=(const QMailTimeStamp& other);

private:
    QSharedDataPointer<QMailTimeStampPrivate> d;
};

#endif
