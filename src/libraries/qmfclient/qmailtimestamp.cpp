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

#include "qmailtimestamp.h"

#include <QDate>
#include <QLocale>
#include <QStringList>
#include <QTime>

#include <stdlib.h>


class QMailTimeStampPrivate : public QSharedData
{
public:
    QMailTimeStampPrivate();
    explicit QMailTimeStampPrivate(const QString& timeText);
    explicit QMailTimeStampPrivate(const QDateTime& dateTime);
    QMailTimeStampPrivate(const QMailTimeStampPrivate& other);
    ~QMailTimeStampPrivate();

    const QMailTimeStampPrivate& operator= (const QMailTimeStampPrivate& other);

    QString toString(QMailTimeStamp::OutputFormat format) const;

    QDateTime toLocalTime() const;
    QDateTime toUTC() const;

    bool isNull() const;
    bool isValid() const;

    bool operator== (const QMailTimeStampPrivate& other) const;
    bool operator!= (const QMailTimeStampPrivate& other) const;

    bool operator< (const QMailTimeStampPrivate& other) const;
    bool operator<= (const QMailTimeStampPrivate& other) const;

    bool operator> (const QMailTimeStampPrivate& other) const;
    bool operator>= (const QMailTimeStampPrivate& other) const;

private:
    // Make these private - since they must be accessed through a smart pointer from the
    // owner class, it's better if all access is restricted to this class
    QDateTime time;
    int utcOffset;
};

QMailTimeStampPrivate::QMailTimeStampPrivate()
    : utcOffset(0)
{
}

QMailTimeStampPrivate::QMailTimeStampPrivate(const QString& timeText)
    : utcOffset(0)
{
    int month = -1, day = -1, year = -1;
    bool ok;
    QString timeStr, offsetStr;

    // Remove any comments from the text
    QString uncommented;
    uncommented.reserve( timeText.length() );

    int commentDepth = 0;
    bool escaped = false;

    const QChar* it = timeText.constData();
    const QChar* end = it + timeText.length();
    for ( ; it != end; ++it ) {
        if ( !escaped && ( *it == QChar::fromLatin1('\\') ) ) {
            escaped = true;
            continue;
        }

        if (*it == QChar::fromLatin1('(') && !escaped) {
            commentDepth += 1;
        } else if (*it == QChar::fromLatin1(')') && !escaped && (commentDepth > 0)) {
            commentDepth -= 1;
        } else if (commentDepth == 0) {
            // Remove characters we don't want
            if ( *it != QChar::fromLatin1(',')
                 && *it != QChar::LineFeed
                 && *it != QChar::CarriageReturn )
                uncommented.append( *it );
        }

        escaped = false;
    }

    // Extract the date/time elements
    uncommented = uncommented.trimmed();
    QStringList tokens = uncommented.split(QChar::Space, Qt::SkipEmptyParts);

    int tokenCount = tokens.count();
    if ( tokenCount > 0 ) {
        static const QString Days(QLatin1String("sunmontuewedthufrisat"));
        if (Days.indexOf(tokens[0].left(3).toLower()) != -1) {
            tokens.removeAt(0);
            tokenCount -= 1;
        }
        if (tokenCount > 0) {
            int value = tokens[0].toInt(&ok);
            if ( ok )
                day = value;
        }
    }
    if ( tokenCount > 1 ) {
        static const QString Months(QLatin1String("janfebmaraprmayjunjulaugsepoctnovdec"));
        int value = Months.indexOf( tokens[1].left( 3 ).toLower() );
        if ( value != -1 )
            month = (value + 3) / 3;
    }
    if ( tokenCount > 2 ) {
        int value = tokens[2].toInt(&ok);
        if ( ok ) {
            // Interpret year according to RFC2822
            year = value;
            if (year < 100) {
                year += (year <= 49 ? 2000 : 1900);
            } else if (year < 1000) {
                year += 1900;
            }
        }
    }
    if ( tokenCount > 3 ) {
        timeStr = tokens[3].trimmed();
    }
    if ( tokenCount > 4 ) {
        offsetStr = tokens[4].trimmed();
    }

    // Parse the text into UTC-adjusted time part and UTC-offset indicator
    if ( (day != -1) && (month != -1) && (year != -1) ) {
        QDate dateComponent(year, month, day);
        QTime timeComponent;

        QTime parsedTime;
        if (timeStr.length() == 8) {
            parsedTime = QTime::fromString(timeStr, QLatin1String("hh:mm:ss"));
        } else if (timeStr.length() == 5) {
            // Is this legal?  Either way, it seems desirable for robustness...
            parsedTime = QTime::fromString(timeStr, QLatin1String("hh:mm"));
        }
        if ( parsedTime.isValid() )
            timeComponent = parsedTime;

        time = QDateTime( dateComponent, timeComponent, Qt::UTC );

        if ( offsetStr.length() == 5 ) {
            int h = offsetStr.left(3).toInt(&ok);
            if ( ok ) {
                int m = offsetStr.right(2).toInt(&ok);
                if ( ok ) {
                    utcOffset = ( h * 3600 ) + ( m * 60 * ( h < 0 ? -1 : 1 ) );

                    time = time.addSecs( -utcOffset );
                }
            }
        }
    }
}

QMailTimeStampPrivate::QMailTimeStampPrivate(const QDateTime& dateTime)
{
    // Store the time as UTC
    if ( dateTime.timeSpec() == Qt::LocalTime ) {
        QDateTime original(dateTime);
        original.setTimeSpec( Qt::UTC );

        time = dateTime.toUTC();

        // Find the difference
        utcOffset = time.secsTo( original );
    } else {
        // Time is already in UTC
        time = dateTime;

        // Is this the right thing to do? What does it mean if we get a UTC timestamp?
        utcOffset = 0;
    }

    // Since we can't include milliseconds in our textual representation, and we
    // need to ensure the following works:
    //    assert( someTimeStamp == QMailTimeStamp(someTimeStamp.toString()) )
    // then we need to make sure that we have no sub-second component
    qint64 ms = time.time().msec();
    if (ms != 0)
        time = time.addMSecs(0 - ms);
}

QMailTimeStampPrivate::QMailTimeStampPrivate(const QMailTimeStampPrivate& other)
    : QSharedData(other)
{
    this->operator=(other);
}

QMailTimeStampPrivate::~QMailTimeStampPrivate()
{
}

const QMailTimeStampPrivate& QMailTimeStampPrivate::operator= (const QMailTimeStampPrivate& other)
{
    time = other.time;
    utcOffset = other.utcOffset;
    return *this;
}

QString QMailTimeStampPrivate::toString(QMailTimeStamp::OutputFormat format) const
{
    // We can't use QDateTime to provide day and month names, since they may get localized into UTF-8
    static const char Days[] = "MonTueWedThuFriSatSun";
    static const char Months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    if (time.isNull() || !time.isValid())
        return QString();

    QString result;

    QDateTime originalTime = time.addSecs( utcOffset );
    QDate originalDate = originalTime.date();

    int hOffset = utcOffset / 3600;
    int mOffset = ( abs(utcOffset) - abs(hOffset * 3600) ) / 60;

    if (format == QMailTimeStamp::Rfc2822) {
        result = QLocale::c().toString(originalTime, QLatin1String("%1, d %2 yyyy hh:mm:ss %3"));
        result = result.arg( QString::fromLatin1( Days + ( originalDate.dayOfWeek() - 1 ) * 3, 3 ) );
        result = result.arg( QString::fromLatin1( Months + ( originalDate.month() - 1 ) * 3, 3 ) );
        result = result.arg( QString().asprintf( "%+.2d%.2d", hOffset, mOffset ) );
    } else if (format == QMailTimeStamp::Rfc3501) {
        result = QLocale::c().toString(originalTime, QLatin1String("dd-%1-yyyy hh:mm:ss %2"));
        result = result.arg( QString::fromLatin1( Months + ( originalDate.month() - 1 ) * 3, 3 ) );
        result = result.arg( QString().asprintf( "%+.2d%.2d", hOffset, mOffset ) );

        // The day number should be space-padded
        if (result[0] == QChar::fromLatin1('0')) {
            result[0] = QChar::Space;
        }
    } else if (format == QMailTimeStamp::Rfc3339) {
        result = QLocale::c().toString(originalTime, QLatin1String("yyyy-MM-ddThh:mm:ss%1"));
        result = result.arg(utcOffset == 0 ? QLatin1String("Z") : QString().asprintf("%+.2d:%.2d", hOffset, mOffset));
    }

    return result;
}

QDateTime QMailTimeStampPrivate::toLocalTime() const
{
    return time.toLocalTime();
}

QDateTime QMailTimeStampPrivate::toUTC() const
{
    return time;
}

bool QMailTimeStampPrivate::isNull() const
{
    return time.isNull();
}

bool QMailTimeStampPrivate::isValid() const
{
    return time.isValid();
}

bool QMailTimeStampPrivate::operator== (const QMailTimeStampPrivate& other) const
{
    return ( toUTC() == other.toUTC() );
}

bool QMailTimeStampPrivate::operator!= (const QMailTimeStampPrivate& other) const
{
    return !operator==(other);
}

bool QMailTimeStampPrivate::operator< (const QMailTimeStampPrivate& other) const
{
    return ( toUTC() < other.toUTC() );
}

bool QMailTimeStampPrivate::operator<= (const QMailTimeStampPrivate& other) const
{
    return !operator>(other);
}

bool QMailTimeStampPrivate::operator> (const QMailTimeStampPrivate& other) const
{
    return ( toUTC() > other.toUTC() );
}

bool QMailTimeStampPrivate::operator>= (const QMailTimeStampPrivate& other) const
{
    return !operator<(other);
}


/*!
    \class QMailTimeStamp

    \brief The QMailTimeStamp class manages message time stamps.
    \ingroup messaginglibrary

    QMailTimeStamp provides functions for creating and manipulating the time stamps of messages.
    QMailTimeStamp can be created from time stamp strings, or from QDateTime objects.  The
    time stamp information can be extracted in UTC time, local time, or as a formatted
    string.

    QMailTimeStamp maintains the timezone information of a time stamp, so it can be used to
    convert time stamp information between UTC time and localized time values.

    \sa QDateTime, QMailMessage
*/

/*!
    \enum QMailTimeStamp::OutputFormat

    This enum type is used to select a format for timestamp output.

    \value Rfc2822  The format used in SMTP message format; example: "Wed, 17 May 2006 20:45:00 +0100".
    \value Rfc3501  The format used in IMAP message append; example: "17-May-2006 20:45:00 +0100".
    \value Rfc3339  The format specified for future protocols (a variant of ISO 8601); example: "2006-05-17T20:45:00+01:00".
*/

/*!
    Returns a QMailTimeStamp object initialised to contain the current
    date and time, in the local time zone.
*/
QMailTimeStamp QMailTimeStamp::currentDateTime()
{
    return QMailTimeStamp(QDateTime::currentDateTime());
}

/*!
    Constructs a null QMailTimeStamp object. A null timestamp is invalid.
*/
QMailTimeStamp::QMailTimeStamp()
{
    d = new QMailTimeStampPrivate();
}

/*! \internal */
QMailTimeStamp::QMailTimeStamp(const QMailTimeStamp& other)
{
    this->operator=(other);
}

/*!
    Constructs a QMailTimeStamp object by parsing \a timeText.
*/
QMailTimeStamp::QMailTimeStamp(const QString& timeText)
{
    d = new QMailTimeStampPrivate(timeText);
}

/*!
    Constructs a QMailTimeStamp object from the given \a dateTime.
*/
QMailTimeStamp::QMailTimeStamp(const QDateTime& dateTime)
{
    d = new QMailTimeStampPrivate(dateTime);
}

/*!
    Destroys a QMailTimeStamp object.
*/
QMailTimeStamp::~QMailTimeStamp()
{
}

/*! \internal */
const QMailTimeStamp& QMailTimeStamp::operator= (const QMailTimeStamp& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns the time stamp information in the format specified by \a format.
*/
QString QMailTimeStamp::toString(QMailTimeStamp::OutputFormat format) const
{
    return d->toString(format);
}

/*!
    Returns a QDateTime containing the time stamp converted to the local time zone.
*/
QDateTime QMailTimeStamp::toLocalTime() const
{
    return d->toLocalTime();
}

/*!
    Returns a QDateTime containing the time stamp in UTC time.
*/
QDateTime QMailTimeStamp::toUTC() const
{
    return d->toUTC();
}

/*!
    Returns true if the timestamp has not been initialized to contain a value.
*/
bool QMailTimeStamp::isNull() const
{
    return d->isNull();
}

/*!
    Returns true if the timestamp is valid; otherwise returns false;
*/
bool QMailTimeStamp::isValid() const
{
    return d->isValid();
}

/*!
    Returns true if this time stamp is equal to \a other; otherwise returns false.

    \sa operator!=()
*/
bool QMailTimeStamp::operator== (const QMailTimeStamp& other) const
{
    return d->operator==(*other.d);
}

/*!
    Returns true if this time stamp is different from \a other; otherwise returns false.

    Two time stamps are different if either the date, the time, or the time zone components are different.

    \sa operator==()
*/
bool QMailTimeStamp::operator!= (const QMailTimeStamp& other) const
{
    return d->operator!=(*other.d);
}

/*!
    Returns true if this time stamp is earlier than \a other; otherwise returns false.
*/
bool QMailTimeStamp::operator< (const QMailTimeStamp& other) const
{
    return d->operator<(*other.d);
}

/*!
    Returns true if this time stamp is earlier than or equal to \a other; otherwise returns false.
*/
bool QMailTimeStamp::operator<= (const QMailTimeStamp& other) const
{
    return d->operator<=(*other.d);
}

/*!
    Returns true if this time stamp is later than \a other; otherwise returns false.
*/
bool QMailTimeStamp::operator> (const QMailTimeStamp& other) const
{
    return d->operator>(*other.d);
}

/*!
    Returns true if this time stamp is later than or equal to \a other; otherwise returns false.
*/
bool QMailTimeStamp::operator>= (const QMailTimeStamp& other) const
{
    return d->operator>=(*other.d);
}
