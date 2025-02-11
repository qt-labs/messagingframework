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

#include "longstream_p.h"
#include "qmaillog.h"
#include "qmailnamespace.h"

#include <QCoreApplication>
#include <QIODevice>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>
#include <QStorageInfo>

/*  Helper class to reduce memory usage while downloading large mails */
LongStream::LongStream()
    : mStatus(Ok)
{
    QString tmpName(QMail::tempPath() + QLatin1String("longstream"));

    len = 0;
    appendedBytes = minCheck;

    tmpFile = new QTemporaryFile( tmpName + QLatin1String( ".XXXXXX" ));
    if (tmpFile->open()) {
        tmpFile->setPermissions(QFile::ReadOwner | QFile::WriteOwner);
        ts = new QDataStream( tmpFile );
    } else {
        qWarning() << "Unable to open temporary file:" << tmpFile->fileName();
        ts = 0;
        setStatus(OutOfSpace);
    }
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

    QString tmpName(QMail::tempPath() + QLatin1String("longstream"));

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
    if (ts) {
        ts->writeRawData(str.toLatin1().constData(), str.length());

        len += str.length();
        appendedBytes += str.length();
        if (appendedBytes >= minCheck) {
            appendedBytes = 0;
            updateStatus();
        }
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

    if (ts) {
        while (!ts->atEnd()) {
            char buffer[1024];
            int len = ts->readRawData(buffer, 1024);
            if (len == -1) {
                break;
            } else {
                result.append(QString::fromLatin1(buffer, len));
            }
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
    long long boundary = minFree;
    if (min >= 0)
        boundary = min;

    QString partitionPath = path.isEmpty() ? QMail::tempPath() : path;

    QStorageInfo storageInfo(partitionPath);
    return storageInfo.bytesAvailable() > boundary;
}

QString LongStream::outOfSpaceMessage()
{
    return QCoreApplication::tr("Storage for messages is full. Some new messages could not be retrieved.");
}

void LongStream::cleanupTempFiles()
{
    QDir dir(QMail::tempPath(), QLatin1String("longstream.*"));
    QStringList list = dir.entryList();
    for (int i = 0; i < list.size(); ++i) {
        QFile file(QMail::tempPath() + list.at(i));
        if (file.exists())
            file.remove();
    }
}
