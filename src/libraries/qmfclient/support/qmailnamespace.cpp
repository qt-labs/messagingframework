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

#include "qmailnamespace.h"
#include "qmailfolderkey.h"
#include "qmailstore.h"
#include "qmaillog.h"
#include <QCoreApplication>
#include <QDir>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QMutex>
#include <QRegularExpression>
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

#include <QLockFile>

static const char* QMF_DATA_ENV="QMF_DATA";
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
    Returns the path to where the Messaging framework will store its data files.
*/
QString QMail::dataPath()
{
    // encoding as best guess, likely just ascii
    static QString dataEnv(QString::fromUtf8(qgetenv(QMF_DATA_ENV)));
    if(!dataEnv.isEmpty())
        return dataEnv + QChar::fromLatin1('/');
    //default to ~/.qmf if not env set
    return QDir::homePath() + QLatin1String("/.qmf/");
}
/*!
    Returns the the time when the Messaging framework store file was las updated.
*/
QDateTime QMail::lastDbUpdated()
{
    static QString database_path(dataPath() + QLatin1String("database"));
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
    return (dataPath() + QLatin1String("tmp/"));
}

/*!
    Returns the path to where the Messaging framework will invoke the messageserver process.
*/
QString QMail::messageServerPath()
{
    static QString serverEnv(QString::fromUtf8(qgetenv(QMF_SERVER_ENV)));
    if(!serverEnv.isEmpty())
        return serverEnv + QChar::fromLatin1('/');

    return QCoreApplication::applicationDirPath() + QChar::fromLatin1('/');
}

/*!
    Returns the path to where the Messaging framework will search for settings information.
*/
QString QMail::messageSettingsPath()
{
    static QString settingsEnv(QString::fromUtf8(qgetenv(QMF_SETTINGS_ENV)));
    if(!settingsEnv.isEmpty())
        return settingsEnv + QChar::fromLatin1('/');
    return QCoreApplication::applicationDirPath() + QChar::fromLatin1('/');
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
        : init(false)
    {
    }

    ~QDatabaseInstanceData()
    {
    }

    QString dbConnectionName()
    {
        return QString::asprintf("qmailstore_sql_connection_%p", this);
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
        QSqlDatabase::removeDatabase(instance->dbConnectionName());
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
        db = QSqlDatabase::database(instance->dbConnectionName());
    } else {
        qMailLog(Messaging) << "opening database";
        db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), instance->dbConnectionName());
        
        QDir dbDir(dataPath() + QLatin1String("database"));
        if (!dbDir.exists()) {
#ifdef Q_OS_UNIX
            QString path = dataPath();
            if (path.endsWith(QChar::fromLatin1('/')))
                path = path.left(path.length() - 1);
            if (!QDir(path).exists() && ::mkdir(QFile::encodeName(path), S_IRWXU) == -1)
                qCritical() << "Cannot create database directory: " << errno;
#endif
            if (!dbDir.mkpath(dataPath() + QLatin1String("database")))
                qCritical() << "Cannot create database path";
        }

        db.setDatabaseName(dataPath() + QLatin1String("database/qmailstore.db"));
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

    QFile file(QLatin1String(":/qmf/mime.types"));
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
    QString mime_sep(QChar::fromLatin1('/'));
    bool doesntLookLikeMimeString = (filename.count(mime_sep) != 1) || (filename[0] == QDir::separator());

    if (doesntLookLikeMimeString || QFile::exists(filename)) {
        int dot = filename.lastIndexOf(QChar::fromLatin1('.'));
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
    Returns the 'base' form of \a subject, using the transformation defined by RFC5256.
    If the original subject contains any variant of the tokens "Re" or "Fwd" recognized by
    RFC5256, then \a replyOrForward will be set to true.
*/
QString QMail::baseSubject(const QString& subject, bool *replyOrForward)
{
    // Implements the conversion from subject to 'base subject' defined by RFC 5256
    QString result(subject);

    bool repeat = false;
    do {
        // Remove any subj-trailer
        QRegularExpression subjTrailer(QLatin1String("(?:"
                                "[ \\t]+"               // WSP
                            "|"
                                "(\\([Ff][Ww][Dd]\\))"    // "(fwd)"
                            ")$"));
        repeat = false;
        do {
            QRegularExpressionMatch match = subjTrailer.match(result);
            if (match.hasMatch()) {
                if (!match.captured(1).isEmpty()) {
                    *replyOrForward = true;
                }
                result = result.left(match.capturedStart());
                repeat = true;
            } else {
                repeat = false;
            }
        } while (repeat);

        bool modified = false;
        do {
            modified = false;

            // Remove any subj-leader
            QRegularExpression subjLeader(QLatin1String("^(?:"
                                    "[ \\t]+"       // WSP
                               "|"
                                    "(?:\\[[^\\[\\]]*\\][ \\t]*)*"        // ( '[' 'blobchar'* ']' WSP* )*
                                    "([Rr][Ee]|[Ff][Ww][Dd]?)[ \\t]*"   // ( "Re" | "Fw" | "Fwd") WSP*
                                    "(?:\\[[^\\[\\]]*\\][ \\t]*)?"        // optional: ( '[' 'blobchar'* ']' WSP* )
                                    ":"                                 // ':'
                               ")"));
            repeat = false;
            do {
                QRegularExpressionMatch match = subjLeader.match(result);
                if (match.hasMatch()) {
                    if (!match.captured(1).isEmpty()) {
                        *replyOrForward = true;
                    }
                    result = result.mid(match.capturedLength());
                    modified = true;
                    repeat = true;
                } else {
                    repeat = false;
                }
            } while (repeat);

            // Remove subj-blob, if there would be a remainder
            QRegularExpression subjBlob(QLatin1String("^(\\[[^\\[\\]]*\\][ \\t]*)"));  // '[' 'blobchar'* ']' WSP*
            QRegularExpressionMatch match = subjBlob.match(result);
            if (match.hasMatch() && (match.captured().length() < result.length())) {
                result = result.mid(match.captured().length());
                modified = true;
            }
        } while (modified);

        // Remove subj-fwd-hdr and subj-fwd-trl if both are present
        QRegularExpressionMatch subjFwdHdr =
            QRegularExpression(QLatin1String("^\\[[Ff][Ww][Dd]:")).match(result);
        QRegularExpressionMatch subjFwdTrl =
            QRegularExpression(QLatin1String("\\]$")).match(result);
        repeat = false;
        if (subjFwdHdr.hasMatch() && subjFwdTrl.hasMatch()) {
            *replyOrForward = true;
            result = result.mid(subjFwdHdr.captured().length(), result.length() - (subjFwdHdr.captured().length() + subjFwdTrl.captured().length()));
            repeat = true;
        }
    } while (repeat);

    return result;
}

static QString normaliseIdentifier(const QString& str)
{
    // Don't permit space, tab or quote marks
    static const QChar skip[] = { QLatin1Char(' '), QLatin1Char('\t'), QLatin1Char('"') };

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

    QRegularExpression identifierPattern(QLatin1String("("
                                "(?:[ \\t]*)"           // Optional leading whitespace
                                "[^ \\t\\<\\>@]+"       // Leading part
                                "(?:[ \\t]*)"           // Optional whitespace allowed before '@'?
                                "@"
                                "(?:[ \\t]*)"           // Optional whitespace allowed after '@'?
                                "[^ \\t\\<\\>]+"        // Trailing part
                              ")"));

    // Extracts message identifiers from \a str, matching the definition used in RFC 5256
    int index = str.indexOf(QChar::fromLatin1('<'));
    if (index != -1) {
        // This may contain other information besides the IDs delimited by < and >
        do {
            // Extract only the delimited content
            QRegularExpressionMatch match = identifierPattern.match(str, index + 1);
            if (match.hasMatch()) {
                result.append(normaliseIdentifier(match.captured(1)));
                index += match.capturedLength();
            } else {
                index += 1;
            }

            index = str.indexOf(QChar::fromLatin1('<'), index);
        } while (index != -1);
    } else {
        // No delimiters - consider the entirety as an identifier
        QRegularExpressionMatch match = identifierPattern.match(str);
        if (match.hasMatch()) {
            result.append(normaliseIdentifier(match.captured(1)));
        }
    }

    return result;
}

QMap<QByteArray, QStringList> standardFolderTranslations()
{
    QMap<QByteArray, QStringList> folderTranslations;

    QFile file(QLatin1String(":/qmf/translations.conf"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to read " << "translations";
        return folderTranslations;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList list = line.split(QLatin1Char('='), Qt::SkipEmptyParts);
        QString folderName = list.at(0);
        QString transList = list.at(1);

        if (folderName == QLatin1String("inbox")) {
            QStringList inboxList = transList.split(QLatin1Char(','), Qt::SkipEmptyParts);
            folderTranslations.insert("inbox", inboxList);
        }
        else if (folderName == QLatin1String("drafts")) {
            QStringList draftsList = transList.split(QLatin1Char(','), Qt::SkipEmptyParts);
            folderTranslations.insert("drafts", draftsList);
        }
        else if (folderName == QLatin1String("trash")) {
            QStringList trashList = transList.split(QLatin1Char(','), Qt::SkipEmptyParts);
            folderTranslations.insert("trash", trashList);
        }
        else if (folderName == QLatin1String("sent")) {
            QStringList sentList = transList.split(QLatin1Char(','), Qt::SkipEmptyParts);
            folderTranslations.insert("sent", sentList);
        }
        else if (folderName == QLatin1String("spam")) {
            QStringList spamList = transList.split(QLatin1Char(','), Qt::SkipEmptyParts);
            folderTranslations.insert("spam", spamList);
        }
    }
    return folderTranslations;
}

QList<StandardFolderInfo> standardFolders()
{
    QList<StandardFolderInfo> standardFoldersList;

    QMap<QByteArray,QStringList> folderTranslations = standardFolderTranslations();

    if (!folderTranslations.empty()) {
        standardFoldersList << StandardFolderInfo(QLatin1String("\\Inbox"), QMailFolder::Incoming, QMailFolder::InboxFolder, QMailMessage::Incoming, folderTranslations.value("inbox"))
                            << StandardFolderInfo(QLatin1String("\\Drafts"), QMailFolder::Drafts, QMailFolder::DraftsFolder, QMailMessage::Draft, folderTranslations.value("drafts"))
                            << StandardFolderInfo(QLatin1String("\\Trash"), QMailFolder::Trash, QMailFolder::TrashFolder, QMailMessage::Trash, folderTranslations.value("trash"))
                            << StandardFolderInfo(QLatin1String("\\Sent"), QMailFolder::Sent, QMailFolder::SentFolder, QMailMessage::Sent, folderTranslations.value("sent"))
                            << StandardFolderInfo(QLatin1String("\\Spam"), QMailFolder::Junk, QMailFolder::JunkFolder, QMailMessage::Junk, folderTranslations.value("spam"));
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

/*
  Returns whether the message server is running or not.
*/
bool QMail::isMessageServerRunning()
{
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    return sessionBus.isConnected() && sessionBus.interface()->isServiceRegistered("org.qt.messageserver");
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
