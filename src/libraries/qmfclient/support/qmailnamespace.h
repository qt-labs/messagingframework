/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
#include <QPair>
#include <QString>
#include <QTime>

#if !defined(Q_OS_WIN) || !defined(_WIN32_WCE)
QT_BEGIN_NAMESPACE

class QSqlDatabase;

QT_END_NAMESPACE
#endif

struct StandardFolderInfo
{
    StandardFolderInfo(QString flagName, quint64 flag, QMailFolder::StandardFolder standardFolder, quint64 messageFlag, QStringList paths)
        :_flagName(flagName), _flag(flag), _standardFolder(standardFolder), _messageFlag(messageFlag), _paths(paths) {};

    QString _flagName;
    quint64 _flag;
    QMailFolder::StandardFolder _standardFolder;
    quint64 _messageFlag;
    QStringList _paths;
};

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

    void closeDatabase();
    QMF_EXPORT QSqlDatabase createDatabase();
#endif

    QMF_EXPORT QString baseSubject(const QString& subject, bool *replyOrForward);
    QMF_EXPORT QStringList messageIdentifiers(const QString& str);
    QMF_EXPORT bool detectStandardFolders(const QMailAccountId &accountId);
    QMF_EXPORT int maximumConcurrentServiceActions();
    QMF_EXPORT int maximumConcurrentServiceActionsPerProcess();
    QMF_EXPORT int maximumPushConnections();
    QMF_EXPORT int databaseAutoCloseTimeout();

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
