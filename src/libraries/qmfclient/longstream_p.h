/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef LONGSTREAM_P_H
#define LONGSTREAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QString>
#include "qmailglobal.h"

QT_BEGIN_NAMESPACE

class QTemporaryFile;
class QDataStream;

QT_END_NAMESPACE

class QMF_EXPORT LongStream
{
public:
    LongStream();
    virtual ~LongStream() ;
    void reset();
    QString detach();
    void append(QString str);
    int length();
    QString fileName();
    QString readAll();

    enum Status { Ok, OutOfSpace };
    Status status();
    void resetStatus();
    void setStatus( Status );
    void updateStatus();
    static bool freeSpace( const QString &path = QString(), int min = -1);

    static QString errorMessage( const QString &prefix = QString());
    static QString tempDir();
    static void cleanupTempFiles();

private:
    QTemporaryFile *tmpFile;
    QDataStream *ts;
    QChar c;
    int len;
    uint appendedBytes;
    Status mStatus;

    static const unsigned long long minFree = 1024*100;
    static const uint minCheck = 1024*10;
};
#endif
