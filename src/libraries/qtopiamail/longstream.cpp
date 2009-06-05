/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/
#include "longstream_p.h"
#include "qmaillog.h"
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
        QFile file( LongStream::tempDir() + "/" + list.at(i) );
        if (file.exists())
            file.remove();
    }
}
