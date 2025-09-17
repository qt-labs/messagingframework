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

#ifndef QMAILNAMESPACE_H
#define QMAILNAMESPACE_H

#include "qmailglobal.h"
#include "qmailaccount.h"
#include "qmailfolder.h"

#include <QDate>
#include <QString>
#include <QTime>

QT_BEGIN_NAMESPACE
class QSqlDatabase;
QT_END_NAMESPACE

namespace QMail
{
    QMF_EXPORT QString dataPath();
    QMF_EXPORT QDateTime lastDbUpdated();
    QMF_EXPORT QString tempPath();
    QMF_EXPORT QString messageServerPath();
    QMF_EXPORT QString messageSettingsPath();

    void closeDatabase();
    QMF_EXPORT QSqlDatabase createDatabase();

    QMF_EXPORT QString baseSubject(const QString& subject, bool *replyOrForward);
    QMF_EXPORT QStringList messageIdentifiers(const QString& str);
    QMF_EXPORT bool detectStandardFolders(const QMailAccountId &accountId);
    QMF_EXPORT int maximumConcurrentServiceActions();
    QMF_EXPORT int maximumConcurrentServiceActionsPerProcess();
    QMF_EXPORT int maximumPushConnections();
    QMF_EXPORT int databaseAutoCloseTimeout();
    QMF_EXPORT bool isMessageServerRunning();

    template<typename StringType> struct qchar_conversion;
    template<> struct qchar_conversion<QString> { static QChar fn(const QChar c) { return c; } };
    template<> struct qchar_conversion<QByteArray> { static QChar fn(const char c) { return QChar::fromLatin1(c); } };

    template<typename StringType> struct ascii_str_conversion;
    template<> struct ascii_str_conversion<QString> { static QString fn(const char *str) { return QString::fromLatin1(str); } };
    template<> struct ascii_str_conversion<QByteArray> { static QByteArray fn(const char *str) { return QByteArray(str); } };

    template<typename StringType>
    StringType unquoteString(const StringType& src)
    {
        // If a string has double-quote as the first and last characters, return the string
        // between those characters
        int length = src.length();
        if (length) {
            typename StringType::const_iterator const begin = src.constData();
            typename StringType::const_iterator const last = begin + length - 1;

            if ((last > begin)
                    && (qchar_conversion<StringType>::fn(*begin) == QChar::fromLatin1('"')
                    &&  qchar_conversion<StringType>::fn(*last)  == QChar::fromLatin1('"')))
                return src.mid(1, length - 2);
        }

        return src;
    }

    template<typename StringType>
    StringType quoteString(const StringType& src)
    {
        StringType result(ascii_str_conversion<StringType>::fn("\"\""));

        // Return the input string surrounded by double-quotes, which are added if not present
        int length = src.length();
        if (length) {
            result.reserve(length + 2);

            typename StringType::const_iterator begin = src.constData();
            typename StringType::const_iterator last = begin + length - 1;

            if (qchar_conversion<StringType>::fn(*begin) == QChar::fromLatin1('"'))
                begin += 1;

            if ((last >= begin) && (qchar_conversion<StringType>::fn(*last) == QChar::fromLatin1('"')))
                last -= 1;

            if (last >= begin)
                result.insert(1, StringType(begin, (last - begin + 1)));
        }

        return result;
    }

    enum SaslMechanism {
        NoMechanism = 0,
        LoginMechanism = 1,
        PlainMechanism = 2,
        CramMd5Mechanism = 3,
        XOAuth2Mechanism = 4
    };
}

#endif
