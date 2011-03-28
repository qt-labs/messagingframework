/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILNAMESPACE_H
#define QMAILNAMESPACE_H

#include "qmailglobal.h"
#include <QDate>
#include <QPair>
#include <QString>
#include <QTime>

#if !defined(Q_OS_WIN) || !defined(_WIN32_WCE)
QT_BEGIN_NAMESPACE

#if defined(SYMBIAN_USE_DATA_CAGED_DATABASE)
class SymbianSqlDatabase;
#else
class QSqlDatabase;
#endif

QT_END_NAMESPACE
#endif

namespace QMail
{
    QMF_EXPORT QString lastSystemErrorMessage();
    QMF_EXPORT void usleep(unsigned long usecs);
    QMF_EXPORT QString dataPath();
    QMF_EXPORT QDateTime lastDbUpdated();
    QMF_EXPORT QString tempPath();
    QMF_EXPORT QString pluginsPath();
    QMF_EXPORT QString sslCertsPath();
    QMF_EXPORT QString messageServerPath();
    QMF_EXPORT QString messageSettingsPath();
    QMF_EXPORT QString messageServerLockFilePath();    
    QMF_EXPORT QString mimeTypeFromFileName(const QString& filename);
    QMF_EXPORT QStringList extensionsForMimeType(const QString& mimeType);

#if !defined(Q_OS_WIN) || !defined(_WIN32_WCE)
    QMF_EXPORT int fileLock(const QString& filePath);
    QMF_EXPORT bool fileUnlock(int id);

#if defined(SYMBIAN_USE_DATA_CAGED_DATABASE)
    SymbianSqlDatabase createDatabase();
#else
    QMF_EXPORT QSqlDatabase createDatabase();
#endif
#endif

    QMF_EXPORT QString baseSubject(const QString& subject, bool *replyOrForward);
    QMF_EXPORT QStringList messageIdentifiers(const QString& str);

    template<typename StringType>
    StringType unquoteString(const StringType& src)
    {
        // If a string has double-quote as the first and last characters, return the string
        // between those characters
        int length = src.length();
        if (length)
        {
            typename StringType::const_iterator const begin = src.constData();
            typename StringType::const_iterator const last = begin + length - 1;

            if ((last > begin) && (*begin == '"' && *last == '"'))
                return src.mid(1, length - 2);
        }

        return src;
    }

    template<typename StringType>
    StringType quoteString(const StringType& src)
    {
        StringType result("\"\"");

        // Return the input string surrounded by double-quotes, which are added if not present
        int length = src.length();
        if (length)
        {
            result.reserve(length + 2);

            typename StringType::const_iterator begin = src.constData();
            typename StringType::const_iterator last = begin + length - 1;

            if (*begin == '"')
                begin += 1;

            if ((last >= begin) && (*last == '"'))
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
        CramMd5Mechanism = 3
    };

}

#endif
