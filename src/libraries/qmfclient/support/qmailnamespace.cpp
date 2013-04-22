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

#include "qmailnamespace.h"
#include "qmailfolderkey.h"
#include "qmailstore.h"
#include "qmaillog.h"
#include <QCoreApplication>
#include <QDir>
#include <QMutex>
#include <QRegExp>
#include <QThreadStorage>
#include <stdio.h>
#if !defined(Q_OS_WIN) || !defined(_WIN32_WCE)
// Not available for windows mobile?
#include <QSqlDatabase>
#include <QSqlError>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

static const char* QMF_DATA_ENV="QMF_DATA";
static const char* QMF_PLUGINS_ENV="QMF_PLUGINS";
static const char* QMF_SERVER_ENV="QMF_SERVER";
static const char* QMF_SETTINGS_ENV="QMF_SETTINGS";

/*!
    \namespace QMail
    \ingroup messaginglibrary

    \brief The QMail namespace contains miscellaneous functionality used by the Messaging framework.
*/

/*!
    \fn StringType QMail::unquoteString(const StringType& src)

    If \a src has double-quote as the first and last characters, return the string between those characters;
    otherwise, return the original string.
*/

/*!
    \fn StringType QMail::quoteString(const StringType& src)

    Returns \a src surrounded by double-quotes, which are added if not already present.
*/

#ifdef Q_OS_WIN
static QMap<int, HANDLE> lockedFiles;
#endif

#if !defined(Q_OS_WIN) || !defined(_WIN32_WCE) // Not supported on windows mobile
/*!
    Convenience function that attempts to obtain a lock on a file with name \a lockFile.
    It is not necessary to create \a lockFile as this file is created temporarily.

    Returns the id of the lockFile if successful or \c -1 for failure.

    \sa QMail::fileUnlock()
*/
int QMail::fileLock(const QString& lockFile)
{
    QString path = QDir::tempPath() + '/' + lockFile;
#ifdef Q_OS_UNIX
    //Store the file in /var/lock instead system's temporary directory
    if (QDir("/var/lock").exists() && QFileInfo("/var/lock").isWritable()) {
        path = QString("/var/lock") + '/' + lockFile;
    }
#endif

#ifdef Q_OS_WIN
    static int lockedCount = 0;

	if (!QFile::exists(path)) {
		QFile file(path);
		file.open(QIODevice::WriteOnly);
		file.close();
	}

    HANDLE handle = ::CreateFile(reinterpret_cast<const wchar_t*>(path.utf16()),
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        qWarning() << "Unable to open file for locking:" << path;
    } else {
        if (::LockFile(handle, 0, 0, 1, 0) == FALSE) {
            qWarning() << "Unable to lock file:" << path;
        } else {
            ++lockedCount;
            lockedFiles.insert(lockedCount, handle);
            return lockedCount;
        }
    }

    return -1;
#else
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int fdlock = -1;
    if((fdlock = ::open(path.toLatin1(), O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
        return -1;

    if(::fcntl(fdlock, F_SETLK, &fl) == -1)
        return -1;

    return fdlock;
#endif
}

/*!
    Convenience function that attempts to unlock the file with identifier \a id that was locked by \c QMail::fileLock.

    Returns \c true for success or \c false otherwise.

    \sa QMail::fileLock()
*/
bool QMail::fileUnlock(int id)
{
#ifdef Q_OS_WIN
    QMap<int, HANDLE>::iterator it = lockedFiles.find(id);
    if (it != lockedFiles.end()) {
        if (::UnlockFile(it.value(), 0, 0, 1, 0) == FALSE) {
            qWarning() << "Unable to unlock file:" << lastSystemErrorMessage();
        } else {
            if (::CloseHandle(it.value()) == FALSE) {
                qWarning() << "Unable to close handle:" << lastSystemErrorMessage();
            }

            lockedFiles.erase(it);
            return true;
        }
    }

    return false;
#else
    struct flock fl;

    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int result = -1;

    result = ::fcntl(id,F_SETLK, &fl);
    if (result == -1)
        return false;

    result = ::close(id);
    if (result == -1)
        return false;

    return true;
#endif
}
#endif

/*!
    Returns the path to where the Messaging framework will store its data files.
*/
QString QMail::dataPath()
{
    static QString dataEnv(qgetenv(QMF_DATA_ENV));
    if(!dataEnv.isEmpty())
        return dataEnv + '/';
    //default to ~/.qmf if not env set
    return QDir::homePath() + "/.qmf/";
}
/*!
    Returns the the time when the Messaging framework store file was las updated.
*/
QDateTime QMail::lastDbUpdated()
{
    static QString database_path(dataPath() + "database");
    QDir dir(database_path);

    if (!dir.exists()) {
        qWarning() << Q_FUNC_INFO << " database dir doesn't exist";
        return QDateTime();
    }

    QStringList entries(dir.entryList(QDir::NoFilter, QDir::Time));

    if (entries.empty()) {
        qWarning() << Q_FUNC_INFO << " found nothing in database dir";
        return QDateTime();
    }

    QFileInfo info(dir, entries.first());

    if (!info.exists()) {
        qWarning() << Q_FUNC_INFO << "Could not open file we just found?";
        return QDateTime();
    }

    return info.lastModified();
}


/*!
    Returns the path to where the Messaging framework will store its temporary files.
*/
QString QMail::tempPath()
{
    return (dataPath() + "tmp/");
}

/*!
    Returns the path to where the Messaging framework will look for its plugin directories
*/
QString QMail::pluginsPath()
{
    static QString pluginsEnv(qgetenv(QMF_PLUGINS_ENV));
    if(!pluginsEnv.isEmpty())
        return pluginsEnv + '/';

    // default to QMF_INSTALL_ROOT/lib/qmf/plugins, as that's where it will most
    // likely be. we also search the old fallback (".") via QCoreApplication,
    // still.
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QString::fromUtf8(QMF_INSTALL_ROOT) + "/lib/qmf/plugins5/";
#else
    return QString::fromUtf8(QMF_INSTALL_ROOT) + "/lib/qmf/plugins/";
#endif
}

/*!
    Returns the path to where the Messaging framework will search for SSL certificates.
*/
QString QMail::sslCertsPath()
{
    return "/etc/ssl/certs/";
}

/*!
    Returns the path to where the Messaging framework will invoke the messageserver process.
*/
QString QMail::messageServerPath()
{
    static QString serverEnv(qgetenv(QMF_SERVER_ENV));
    if(!serverEnv.isEmpty())
        return serverEnv + '/';

    return QCoreApplication::applicationDirPath() + '/';
}

/*!
    Returns the path to where the Messaging framework will search for settings information.
*/
QString QMail::messageSettingsPath()
{
    static QString settingsEnv(qgetenv(QMF_SETTINGS_ENV));
    if(!settingsEnv.isEmpty())
        return settingsEnv + '/';
    return QCoreApplication::applicationDirPath() + '/';
}

/*!
  Returns the full path to the file used to ensure that only one instance of the 
  messageserver is running.
*/
QString QMail::messageServerLockFilePath()
{
    static QString path(QDir::tempPath() + QString("/messageserver-instance.lock"));
    //check unix path
#ifdef Q_OS_UNIX
    //Store the file in /var/lock instead system's temporary directory
    if (QDir("/var/lock").exists() && QFileInfo("/var/lock").isWritable()) {
        path = "/var/lock/messageserver-instance.lock";
    }
#endif
    return path;
}


#if !defined(Q_OS_WIN) || !defined(_WIN32_WCE) // Not supported on windows mobile
/*!
    Returns the database where the Messaging framework will store its message meta-data. 
    If the database does not exist, it is created.
*/
#ifdef Q_OS_UNIX
// for mkdir
#include <sys/types.h>
#include <sys/stat.h>
#endif

class QDatabaseInstanceData
{
public:
    QDatabaseInstanceData()
    {
    init = false;
    }

    ~QDatabaseInstanceData()
    {
    }

    bool init;
};

Q_GLOBAL_STATIC(QThreadStorage<QDatabaseInstanceData *>, databaseDataInstance)

void QMail::closeDatabase()
{
    QDatabaseInstanceData* instance = databaseDataInstance()->localData();

    if (instance->init) {
        qMailLog(Messaging) << "closing database";
        instance->init = false;
        QSqlDatabase::removeDatabase("qmailstore_sql_connection");
    } // else nothing todo
}

QSqlDatabase QMail::createDatabase()
{
    if (!databaseDataInstance()->hasLocalData()) {
        databaseDataInstance()->setLocalData(new QDatabaseInstanceData);
    }
    QDatabaseInstanceData* instance = databaseDataInstance()->localData();

    QSqlDatabase db;
    if (instance->init) {
        db = QSqlDatabase::database("qmailstore_sql_connection");
    } else {
        qMailLog(Messaging) << "opening database";
        db = QSqlDatabase::addDatabase("QSQLITE", "qmailstore_sql_connection");
        
        QDir dbDir(dataPath() + "database");
        if (!dbDir.exists()) {
#ifdef Q_OS_UNIX
            QString path = dataPath();
            if (path.endsWith('/'))
                path = path.left(path.length() - 1);
            if (!QDir(path).exists() && ::mkdir(QFile::encodeName(path), S_IRWXU) == -1)
                qCritical() << "Cannot create database directory: " << errno;
#endif
            if (!dbDir.mkpath(dataPath() + "database"))
                qCritical() << "Cannot create database path";
        }

        db.setDatabaseName(dataPath() + "database/qmailstore.db");
#endif

        if(!db.open()) {
            QSqlError dbError = db.lastError();
            qCritical() << "Cannot open database: " << dbError.text();
        }

        QDir tp(tempPath());
        if(!tp.exists())
            if(!tp.mkpath(tempPath()))
                qCritical() << "Cannot create temp path";

        instance->init = true;
    }

    return db;
}

/*!
    \internal
    Returns the next word, given the input and starting position.
*/
static QString nextString( const char *line, int& posn )
{
    if ( line[posn] == '\0' )
        return QString();
    int end = posn;
    char ch;
    for (;;) {
        ch = line[end];
        if ( ch == '\0' || ch == ' ' || ch == '\t' ||
             ch == '\r' || ch == '\n' ) {
            break;
        }
        ++end;
    }
    const char *result = line + posn;
    int resultLen = end - posn;
    for (;;) {
        ch = line[end];
        if ( ch == '\0' )
            break;
        if ( ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' )
            break;
        ++end;
    }
    posn = end;
    return QString::fromLocal8Bit(result, resultLen);
}

typedef QHash<QString, QString> typeForType;
Q_GLOBAL_STATIC(typeForType, typeFor);
typedef QHash<QString, QStringList> extForType;
Q_GLOBAL_STATIC(extForType, extFor);

/*!
    \internal
    Loads the mime type to extensions mapping
*/
static void loadExtensions()
{
    QMutex mutex;
    mutex.lock();
    static bool loaded = false;

    if(loaded)
    {
        mutex.unlock();
        return;
    }

    QFile file(":/qmf/mime.types");
    if ( file.open(QIODevice::ReadOnly) ) {
        char line[1024];

        while (file.readLine(line, sizeof(line)) > 0) {
            if (line[0] == '\0' || line[0] == '#')
                continue;
            int posn = 0;
            QString id = nextString(line, posn);
            if ( id.isEmpty() )
                continue;
            id = id.toLower();

            QStringList exts = extFor()->value(id);

            for( QString ext = nextString( line, posn ); !ext.isEmpty(); ext = nextString(line, posn).toLower() )
            {
                if( !exts.contains( ext ) )
                {
                    exts.append( ext );

                    typeFor()->insert(ext, id);
                }
            }
            (*extFor())[ id ] = exts;
        }
        loaded = true;
    }
    mutex.unlock();
}

/*!
    Returns the string mime type based on the filename \a filename.
*/
QString QMail::mimeTypeFromFileName(const QString& filename)
{
    if (filename.isEmpty())
        return QString();

    loadExtensions();

    // do a case insensitive search for a known mime type.
    QString lwrExtOrId = filename.toLower();
    QHash<QString,QStringList>::const_iterator it = extFor()->find(lwrExtOrId);
    if (it != extFor()->end()) {
        return lwrExtOrId;
    }

    // either it doesn't have exactly one mime-separator, or it has
    // a path separator at the beginning
    QString mime_sep('/');
    bool doesntLookLikeMimeString = (filename.count(mime_sep) != 1) || (filename[0] == QDir::separator());

    if (doesntLookLikeMimeString || QFile::exists(filename)) {
        int dot = filename.lastIndexOf('.');
        QString ext = dot >= 0 ? filename.mid(dot+1) : filename;

        QHash<QString,QString>::const_iterator it = typeFor()->find(ext.toLower());
        if (it != typeFor()->end()) {
            return *it;
        }

        const char elfMagic[] = { '\177', 'E', 'L', 'F', '\0' };
        QFile ef(filename);
        if (ef.exists() && (ef.size() > 5) && ef.open(QIODevice::ReadOnly) && (ef.peek(5) == elfMagic)) { // try to find from magic
            return QLatin1String("application/x-executable");  // could be a shared library or an exe
        } else {
            return QLatin1String("application/octet-stream");
        }
    }

    // could be something like application/vnd.oma.rights+object
    return lwrExtOrId;
}

/*!
    Returns a list of valid file extensions for the mime type string \a mimeType
    or an empty list if the mime type is unrecognized.
*/
QStringList QMail::extensionsForMimeType(const QString& mimeType)
{
    loadExtensions();
    return extFor()->value(mimeType);
}

/*!
    Suspends the current process for \a usecs microseconds.
*/
void QMail::usleep(unsigned long usecs)
{
#ifdef Q_OS_WIN
    ::Sleep((usecs + 500) / 1000);
#else
    static const int factor(1000 * 1000);

    unsigned long seconds(usecs / factor);
    usecs = (usecs % factor);

    if (seconds) {
        ::sleep(seconds);
    }
    if (!seconds || usecs) {
        ::usleep(usecs);
    }
#endif
}

/*!
    Returns the 'base' form of \a subject, using the transformation defined by RFC5256.
    If the original subject contains any variant of the tokens "Re" or "Fwd" recognized by
    RFC5256, then \a replyOrForward will be set to true.
*/
QString QMail::baseSubject(const QString& subject, bool *replyOrForward)
{
    // Implements the conversion from subject to 'base subject' defined by RFC 5256
    int pos = 0;
    QString result(subject);

    bool repeat = false;
    do {
        repeat = false;

        // Remove any subj-trailer
        QRegExp subjTrailer("(?:"
                                "[ \\t]+"               // WSP
                            "|"
                                "(\\([Ff][Ww][Dd]\\))"    // "(fwd)"
                            ")$");
        while ((pos = subjTrailer.indexIn(result)) != -1) {
            if (!subjTrailer.cap(1).isEmpty()) {
                *replyOrForward = true;
            }
            result = result.left(pos);
        }

        bool modified = false;
        do {
            modified = false;

            // Remove any subj-leader
            QRegExp subjLeader("^(?:"
                                    "[ \\t]+"       // WSP
                               "|"
                                    "(?:\\[[^\\[\\]]*\\][ \\t]*)*"        // ( '[' 'blobchar'* ']' WSP* )*
                                    "([Rr][Ee]|[Ff][Ww][Dd]?)[ \\t]*"   // ( "Re" | "Fw" | "Fwd") WSP*
                                    "(?:\\[[^\\[\\]]*\\][ \\t]*)?"        // optional: ( '[' 'blobchar'* ']' WSP* )
                                    ":"                                 // ':'
                               ")");
            while ((pos = subjLeader.indexIn(result)) == 0) {
                if (!subjLeader.cap(1).isEmpty()) {
                    *replyOrForward = true;
                }
                result = result.mid(subjLeader.cap(0).length());
                modified = true;
            }

            // Remove subj-blob, if there would be a remainder
            QRegExp subjBlob("^(\\[[^\\[\\]]*\\][ \\t]*)");             // '[' 'blobchar'* ']' WSP*
            if ((subjBlob.indexIn(result) == 0) && (subjBlob.cap(0).length() < result.length())) {
                result = result.mid(subjBlob.cap(0).length());
                modified = true;
            }
        } while (modified);

        // Remove subj-fwd-hdr and subj-fwd-trl if both are present
        QRegExp subjFwdHdr("^\\[[Ff][Ww][Dd]:");
        QRegExp subjFwdTrl("\\]$");
        if ((subjFwdHdr.indexIn(result) == 0) && (subjFwdTrl.indexIn(result) != -1)) {
            *replyOrForward = true;
            result = result.mid(subjFwdHdr.cap(0).length(), result.length() - (subjFwdHdr.cap(0).length() + subjFwdTrl.cap(0).length()));
            repeat = true;
        }
    } while (repeat);

    return result;
}

static QString normaliseIdentifier(const QString& str)
{
    // Don't permit space, tab or quote marks
    static const QChar skip[] = { QChar(' '), QChar('\t'), QChar('"') };

    QString result;
    result.reserve(str.length());

    QString::const_iterator it = str.begin(), end = str.end();
    while (it != end) {
        if ((*it != skip[0]) && (*it != skip[1]) && (*it != skip[2])) {
            result.append(*it);
        }
        ++it;
    }

    return result;
}

/*!
    Returns the sequence of message identifiers that can be extracted from \a aStr.
    Message identifiers must conform to the definition given by RFC 5256.
*/
QStringList QMail::messageIdentifiers(const QString& aStr)
{
    QStringList result;
    QString str(aStr.left(1000)); // Handle long strings quickly

    QRegExp identifierPattern("("
                                "(?:[ \\t]*)"           // Optional leading whitespace
                                "[^ \\t\\<\\>@]+"       // Leading part
                                "(?:[ \\t]*)"           // Optional whitespace allowed before '@'?
                                "@"
                                "(?:[ \\t]*)"           // Optional whitespace allowed after '@'?
                                "[^ \\t\\<\\>]+"        // Trailing part
                              ")");

    // Extracts message identifiers from \a str, matching the definition used in RFC 5256
    int index = str.indexOf('<');
    if (index != -1) {
        // This may contain other information besides the IDs delimited by < and >
        do {
            // Extract only the delimited content
            if (str.indexOf(identifierPattern, index + 1) == (index + 1)) {
                result.append(normaliseIdentifier(identifierPattern.cap(1)));
                index += identifierPattern.cap(0).length();
            } else {
                index += 1;
            }

            index = str.indexOf('<', index);
        } while (index != -1);
    } else {
        // No delimiters - consider the entirety as an identifier
        if (str.indexOf(identifierPattern) != -1) {
            result.append(normaliseIdentifier(identifierPattern.cap(1)));
        }
    }

    return result;
}

/*!
    Returns the text describing the last error reported by the underlying platform.
*/
QString QMail::lastSystemErrorMessage()
{
#ifdef Q_OS_WIN
    LPVOID buffer;

    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    ::GetLastError(),
                    0,
                    reinterpret_cast<LPTSTR>(&buffer),
                    0,
                    NULL);

    QString result(QString::fromUtf16(reinterpret_cast<const ushort*>(buffer)));
    ::LocalFree(buffer);

    return result;
#else
    return QString(::strerror(errno));
#endif
}

QMap<QString, QStringList> standardFolderTranslations()
{
    QMap<QString, QStringList> folderTranslations;

    QFile file(":/qmf/translations.conf");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to read " << "translations";
        return folderTranslations;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList list = line.split("=", QString::SkipEmptyParts);
        QString folderName = list.at(0);
        QString transList = list.at(1);

        if (folderName == "inbox") {
            QStringList inboxList = transList.split(",", QString::SkipEmptyParts);
            folderTranslations.insert("inbox", inboxList);
        }
        else if (folderName == "drafts") {
            QStringList draftsList = transList.split(",", QString::SkipEmptyParts);
            folderTranslations.insert("drafts", draftsList);
        }
        else if(folderName == "trash") {
            QStringList trashList = transList.split(",", QString::SkipEmptyParts);
            folderTranslations.insert("trash", trashList);
        }
        else if (folderName == "sent") {
            QStringList sentList = transList.split(",", QString::SkipEmptyParts);
            folderTranslations.insert("sent", sentList);
        }
        else if (folderName == "spam") {
            QStringList spamList = transList.split(",", QString::SkipEmptyParts);
            folderTranslations.insert("spam", spamList);
        }
    }
    return folderTranslations;
}

QList<StandardFolderInfo> standardFolders()
{
    QList<StandardFolderInfo> standardFoldersList;

    QMap<QString,QStringList> folderTranslations = standardFolderTranslations();

    if (!folderTranslations.empty()) {
        standardFoldersList << StandardFolderInfo("\\Inbox", QMailFolder::Incoming, QMailFolder::InboxFolder, QMailMessage::Incoming, folderTranslations.value("inbox"))
                            << StandardFolderInfo("\\Drafts", QMailFolder::Drafts, QMailFolder::DraftsFolder, QMailMessage::Draft, folderTranslations.value("drafts"))
                            << StandardFolderInfo("\\Trash", QMailFolder::Trash, QMailFolder::TrashFolder, QMailMessage::Trash, folderTranslations.value("trash"))
                            << StandardFolderInfo("\\Sent", QMailFolder::Sent, QMailFolder::SentFolder, QMailMessage::Sent, folderTranslations.value("sent"))
                            << StandardFolderInfo("\\Spam", QMailFolder::Junk, QMailFolder::JunkFolder, QMailMessage::Junk, folderTranslations.value("spam"));
    }
    return standardFoldersList;
}

bool detectStandardFolder(const QMailAccountId &accountId, StandardFolderInfo standardFolderInfo)
{
    QMailFolderId folderId;
    QMailAccount account = QMailAccount(accountId);

    QMailFolderKey accountKey(QMailFolderKey::parentAccountId(accountId));
    QStringList paths = standardFolderInfo._paths;
    QMailFolder::StandardFolder standardFolder(standardFolderInfo._standardFolder);
    quint64 messageFlag(standardFolderInfo._messageFlag);
    quint64 flag(standardFolderInfo._flag);

    QMailFolderIdList folders;

    if (!paths.isEmpty()) {
        QMailFolderKey exactMatchKey = QMailFolderKey::displayName(paths, QMailDataComparator::Includes);
        folders = QMailStore::instance()->queryFolders(exactMatchKey & accountKey);
        if (folders.isEmpty()) {
            QMailFolderKey pathKey;
            foreach (const QString& path, paths) {
                pathKey |= QMailFolderKey::displayName(path, QMailDataComparator::Includes);
            }
            folders = QMailStore::instance()->queryFolders(pathKey & accountKey);
        }
    }

    if (!folders.isEmpty()) {
        folderId = folders.first();

        if (folderId.isValid()) {
            qMailLog(Messaging) << "Setting folder: " << QMailFolder(folderId).displayName();
            QMailFolder folder(folderId);
            folder.setStatus(flag,true);
            account.setStandardFolder(standardFolder, folderId);
            if (!QMailStore::instance()->updateAccount(&account)) {
                qWarning() << "Unable to update account" << account.id() << "to set standard folder" << QMailFolder(folderId).displayName();
            }
            QMailMessageKey folderKey(QMailMessageKey::parentFolderId(folderId));
            if (!QMailStore::instance()->updateMessagesMetaData(folderKey, messageFlag, true)) {
                qWarning() << "Unable to update messages in folder" << folderId << "to set flag" << messageFlag;
            }
            if (!QMailStore::instance()->updateFolder(&folder)) {
                qWarning() << "Unable to update folder" << folderId;
            }
            return true;
        }
    }
    return false;
}

/*!
    Detects standard folders for the account specified by \a accountId, and
    updates the mail store if standard folders are found.
    
    Detection is based on matching folder names, that is QMailFolder::displayName() 
    against a predefined list of translations.
  
    Returns true if all standard folders are detected; otherwise returns false;
*/
bool QMail::detectStandardFolders(const QMailAccountId &accountId)
{
    QMailAccount account = QMailAccount(accountId);
    bool status = true;
    QList<StandardFolderInfo> standardFoldersList = standardFolders();

    if (standardFoldersList.empty()) {
        return true;
    }

    foreach (StandardFolderInfo folder, standardFoldersList) {
        QMailFolderId standardFolderId = account.standardFolder(folder._standardFolder);
        if (!standardFolderId.isValid()) {
            if (!detectStandardFolder(accountId, folder)) {
                status = false;
            }
        }
    }
    QMailStore::instance()->flushIpcNotifications();
    return status;
}

/*
  Returns the maximum number of service actions that can be serviced
  concurrently on the device. Service actions that can't be serviced
  immediately are queued until an appropriate service becomes available.
  
  Used to limit peak memory (RAM) used by the messageserver.
*/
int QMail::maximumConcurrentServiceActions()
{
    return 2;
}

/*
  Returns the maximum number of service actions that can be serviced
  concurrently per process. Service actions that can't be serviced
  immediately are queued until an appropriate service becomes available.

  Used by the messageserver to prevent a client form monopolizing usage 
  of shared services.
*/
int QMail::maximumConcurrentServiceActionsPerProcess()
{
    return 1;
}

/*
  Returns the maximum number of push connections that may
  be started by the message server process.
*/
int QMail::maximumPushConnections()
{
    return 10;
}

/*
  Returns the number of milliseconds that the database can be unused before
  it will be automatically closed to reduce RAM use.
*/
int QMail::databaseAutoCloseTimeout()
{
    return 600*1000;
}

/*!
    \enum QMail::SaslMechanism

    This enum type describes the available SASL (Simple Authentication and Security Layer \l{http://www.ietf.org/rfc/rfc4422.txt} {RFC 2822} ) 
    mechanisms for authenticating with external servers using protocol plugins. They should be used in conjunction with a data security 
    mechanism such as TLS (Transport Layer Security \l{http://www.ietf.org/rfc4346} {RFC 4346})

    \value NoMechanism No SASL mechanism will be used.
    \value LoginMechanism Simple clear-text user/password authentication mechanism, obsoleted by Plain.
    \value PlainMechanism Simple clear-text user/password authentication mechanism.
    \value CramMd5Mechanism A challenge-response authentication mechanism.
*/
