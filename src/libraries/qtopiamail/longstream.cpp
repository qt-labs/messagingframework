/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/
#include "longstream_p.h"
#include <QApplication>
#include "qmailnamespace.h"
#include <QIODevice>
#include <QTextStream>
#include <QTemporaryFile>
#include <sys/vfs.h>
#include <QDir>

/*  Helper class to reduce memory usage while downloading large mails */
LongStream::LongStream()
{
    QString tmpName( LongStream::tempDir() + QLatin1String( "/qtopiamail" ) );

    tmpFile = new QTemporaryFile( tmpName + QLatin1String( ".XXXXXX" ));
    tmpFile->open(); // todo error checking
    tmpFile->setPermissions(QFile::ReadOwner | QFile::WriteOwner);

    ts = new QDataStream( tmpFile );
    len = 0;
    appendedBytes = minCheck;
}

LongStream::~LongStream()
{
    tmpFile->close();
    delete ts;
    delete tmpFile;
}

void LongStream::reset()
{
    delete ts;

    tmpFile->resize( 0 );
    tmpFile->close();
    tmpFile->open();

    ts = new QDataStream( tmpFile );
    len = 0;
    appendedBytes = minCheck;

    c = QChar::Null;
    resetStatus();
}

QString LongStream::detach()
{
    QString detachedName = fileName();

    delete ts;

    tmpFile->setAutoRemove(false);
    tmpFile->close();
    delete tmpFile;

    QString tmpName( LongStream::tempDir() + QLatin1String( "/qtopiamail" ) );

    tmpFile = new QTemporaryFile( tmpName + QLatin1String( ".XXXXXX" ));
    tmpFile->open();
    tmpFile->setPermissions(QFile::ReadOwner | QFile::WriteOwner);

    ts = new QDataStream( tmpFile );
    len = 0;
    appendedBytes = minCheck;

    c = QChar::Null;
    resetStatus();

    return detachedName;
}

void LongStream::append(QString str)
{
    ts->writeRawData(str.toAscii().constData(), str.length());

    len += str.length();
    appendedBytes += str.length();
    if (appendedBytes >= minCheck) {
        appendedBytes = 0;
        updateStatus();
    }
}

int LongStream::length()
{
    return len;
}

QString LongStream::fileName()
{
    return tmpFile->fileName();
}

QString LongStream::readAll()
{
    QString result;

    while (!ts->atEnd()) {
        char buffer[1024];
        int len = ts->readRawData(buffer, 1024);
        if (len == -1) {
            break;
        } else {
            result.append(QString::fromAscii(buffer, len));
        }
    }

    return result;
}

LongStream::Status LongStream::status()
{
    return mStatus;
}

void LongStream::resetStatus()
{
    mStatus = Ok;
}

void LongStream::updateStatus()
{
    if (!freeSpace())
        setStatus( LongStream::OutOfSpace );
}

void LongStream::setStatus( Status status )
{
    mStatus = status;
}

bool LongStream::freeSpace( const QString &path, int min)
{
    unsigned long long boundary = minFree;
    if (min >= 0)
        boundary = min;
    struct statfs stats;
    QString partitionPath = tempDir() + "/.";
    if (!path.isEmpty())
        partitionPath = path;

    statfs( QString( partitionPath ).toLocal8Bit(), &stats);
    unsigned long long bavail = ((unsigned long long)stats.f_bavail);
    unsigned long long bsize = ((unsigned long long)stats.f_bsize);
    return (bavail * bsize) > boundary;
}

QString LongStream::errorMessage( const QString &prefix )
{
    QString str = QApplication::tr( "Storage for messages is full. Some new "
                                    "messages could not be retrieved." );
    if (!prefix.isEmpty())
        return prefix + str;
    return str;
}

static QString tempDirPath()
{
    QString path = QMail::tempPath();
    QDir dir;
    if (!dir.exists( path ))
        dir.mkpath( path );
    return path;
}

QString LongStream::tempDir()
{
    static QString path(tempDirPath());
    return path;
}

void LongStream::cleanupTempFiles()
{
    QDir dir( LongStream::tempDir(), "qtopiamail.*" );
    QStringList list = dir.entryList();
    for (int i = 0; i < list.size(); ++i) {
        QFile file( LongStream::tempDir() + list.at(i) );
        if (file.exists())
            file.remove();
    }
}
