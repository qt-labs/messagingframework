/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailnamespace.h"
#include <QSqlDatabase>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <QDir>
#include <QDebug>
#include <errno.h>
#include <QDir>
#include <QtDebug>
#include <QMutex>
#include <QRegExp>
#include <unistd.h>
#include <stdlib.h>

static const char* QMF_DATA_ENV="QMF_DATA";
static const char* QMF_PLUGINS_ENV="QMF_PLUGINS";

/*!
  \namespace QMail

  \brief The QMail namespace contains miscellaneous functionality used by the Messaging framework.
*/

/*!
  \fn void QMail::usleep(unsigned long usecs)

  Suspends the current process for \a usecs microseconds.
*/

/*!
  \fn QSqlDatabase QMail::createDatabase()

  Returns the database where the Messaging framework will store its message meta-data. If the database
  does not exist, it is created.
*/

/*!
  \fn QString QMail::dataPath()

  Returns the path to where the Messaging framework will store its data files.
*/

/*!
  \fn QString QMail::tempPath()

  Returns the path to where the Messaging framework will store its temporary files.
*/

/*!
  \fn QString QMail::pluginsPath()

  Returns the path to where the Messaging framework will look for its plugin directories
*/

/*!
  \fn QString QMail::sslCertsPath()

  Returns the path to where the Messaging framework will search for SSL certificates.
*/

/*!
  \fn QString QMail::mimeTypeFromFileName(const QString& filename)

  Returns the string mime type based on the filename \a filename.
*/

/*!
  \fn QStringList QMail::extensionsForMimeType(const QString& mimeType)

  Returns a list of valid file extensions for the mime type string \a mimeType
  or an empty list if the mime type is unrecognized.
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

/*!
    \fn int QMail::fileLock(const QString& lockFile)

    Convenience function that attempts to obtain a lock on a file with name \a lockFile.
    It is not necessary to create \a lockFile as this file is created temporarily.

    Returns the id of the lockFile if successful or \c -1 for failure.

    \sa QMail::fileUnlock()
*/

/*!
    \fn bool QMail::fileUnlock(int id)

    Convenience function that attempts to unlock the file with identifier \a id that was locked by \c QMail::fileLock.

    Returns \c true for success or \c false otherwise.

    \sa QMail::fileLock()
*/


int QMail::fileLock(const QString& lockFile)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int fdlock = -1;

    QString path = QDir::tempPath() + "/" + lockFile;
    if((fdlock = ::open(path.toLatin1(), O_WRONLY|O_CREAT, 0666)) == -1)
        return -1;

    if(::fcntl(fdlock, F_SETLK, &fl) == -1)
        return -1;

    return fdlock;
}

bool QMail::fileUnlock(int id)
{
    struct flock fl;

    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    int result = -1;

    if((result = ::fcntl(id,F_SETLK, &fl)) == -1)
        return false;

    if((result = ::close(id)) == -1)
        return false;

    return true;
}


QString QMail::dataPath()
{
    static QString dataEnv(getenv(QMF_DATA_ENV));
    if(!dataEnv.isEmpty())
        return dataEnv + "/";
    //default to ~/.qmf if not env set
    return QDir::homePath() + "/.qmf";
}

QString QMail::tempPath()
{
    return QDir::tempPath();
}

QString QMail::pluginsPath()
{
    static QString pluginsEnv(getenv(QMF_PLUGINS_ENV));
    if(!pluginsEnv.isEmpty())
        return pluginsEnv + "/";
    //default to "." if no env set
    return pluginsEnv;
}

QString QMail::sslCertsPath()
{
    return "/etc/ssl/certs";
}

QSqlDatabase QMail::createDatabase()
{
    static bool init = false;
    QSqlDatabase db;
    if(!init)
    {
        db = QSqlDatabase::addDatabase("QSQLITE");
        QDir dp(dataPath());
        if(!dp.exists())
            if(!dp.mkpath(dataPath()))
                qCritical() << "Cannot create data path";
        db.setDatabaseName(dataPath() + "/qmailstore.db");
        if(!db.open())
            qCritical() << "Cannot open database";
        else
            init = true;
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
        return QString::null;
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

    QFile file(":qtopiamail/mime.types");
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

QString QMail::mimeTypeFromFileName(const QString& filename)
{
    if (filename.isEmpty())
        return QString();

    loadExtensions();

    QString mime_sep = QLatin1String("/");

    // either it doesnt have exactly one mime-separator, or it has
    // a path separator at the beginning
    //
    bool doesntLookLikeMimeString =
        filename.count( mime_sep ) != 1 ||
        filename[0] == QDir::separator();

    // do a case insensitive search for a known mime type.
    QString lwrExtOrId = filename.toLower();
    QHash<QString,QStringList>::const_iterator it = extFor()->find(lwrExtOrId);
    if ( it != extFor()->end() ) {
        return lwrExtOrId;
    } else if ( doesntLookLikeMimeString || QFile(filename).exists() ) {
        QFile ef(filename);
        int dot = filename.lastIndexOf('.');
        QString ext = dot >= 0 ? filename.mid(dot+1) : filename;
        return typeFor()->value(ext.toLower());
        const char elfMagic[] = { '\177', 'E', 'L', 'F', '\0' };
        if ( ef.exists() && ef.size() > 5 && ef.open(QIODevice::ReadOnly) && ef.peek(5) == elfMagic)  // try to find from magic
            return QLatin1String("application/x-executable");  // could be a shared library or an exe
        else
            return QLatin1String("application/octet-stream");
    }
    else  // could be something like application/vnd.oma.rights+object
    {
        return lwrExtOrId;
    }
}

QStringList QMail::extensionsForMimeType(const QString& mimeType)
{
    loadExtensions();
    return extFor()->value(mimeType);
}

void QMail::usleep(unsigned long usecs)
{
    if ( usecs >= 1000000 )
        ::sleep( usecs / 1000000 );
    ::usleep( usecs % 1000000 );
}

QString QMail::baseSubject(const QString& subject)
{
    // Implements the conversion from subject to 'base subject' defined by RFC 5256
    int pos = 0;
    QString result(subject);

    bool repeat = false;
    do {
        repeat = false;

        // Remove any subj-trailer
        QRegExp subjTrailer("("
                                "[ \\t]+"               // WSP
                            "|"
                                "\\([Ff][Ww][Dd]\\)"    // "(fwd)"
                            ")$");
        while ((pos = subjTrailer.indexIn(result)) != -1) {
            result = result.left(pos);
        }

        bool modified = false;
        do {
            modified = false;

            // Remove any subj-leader
            QRegExp subjLeader("^("
                                    "[ \\t]+"       // WSP
                               "|"
                                    "(\\[[^\\[\\]]*\\][ \\t]*)*"        // ( '[' 'blobchar'* ']' WSP* )*
                                    "([Rr][Ee]|[Ff][Ww][Dd]?)[ \\t]*"   // ( "Re" | "Fw" | "Fwd") WSP*
                                    "(\\[[^\\[\\]]*\\][ \\t]*)?"        // optional: ( '[' 'blobchar'* ']' WSP* )
                                    ":"                                 // ':'
                               ")");
            while ((pos = subjLeader.indexIn(result)) == 0) {
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

QStringList QMail::messageIdentifiers(const QString& str)
{
    QStringList result;

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

