/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILNAMESPACE_H
#define QMAILNAMESPACE_H

#include <QString>
#include "qmailglobal.h"
#include <QDate>
#include <QTime>
#include <QPair>

class QSqlDatabase;

namespace QMail
{
    QTOPIAMAIL_EXPORT uint processId();
    QTOPIAMAIL_EXPORT QString lastSystemErrorMessage();
    QTOPIAMAIL_EXPORT void usleep(unsigned long usecs);
    QTOPIAMAIL_EXPORT QSqlDatabase createDatabase();
    QTOPIAMAIL_EXPORT QString dataPath();
    QTOPIAMAIL_EXPORT QString tempPath();
    QTOPIAMAIL_EXPORT QString pluginsPath();
    QTOPIAMAIL_EXPORT QString sslCertsPath();
    QTOPIAMAIL_EXPORT QString messageServerPath();
    QTOPIAMAIL_EXPORT QString mimeTypeFromFileName(const QString& filename);
    QTOPIAMAIL_EXPORT QStringList extensionsForMimeType(const QString& mimeType);
    QTOPIAMAIL_EXPORT int fileLock(const QString& filePath);
    QTOPIAMAIL_EXPORT bool fileUnlock(int id);

    QTOPIAMAIL_EXPORT QString baseSubject(const QString& subject);
    QTOPIAMAIL_EXPORT QStringList messageIdentifiers(const QString& str);

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

}

#endif
