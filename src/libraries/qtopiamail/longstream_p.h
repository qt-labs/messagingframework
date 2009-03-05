/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

class QTemporaryFile;
class QTextStream;

class QTOPIAMAIL_EXPORT LongStream
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
    QString readLine();
    QString first();
    QString next();
    QString current();

    enum Status { Ok, OutOfSpace };
    Status status();
    void resetStatus();
    void setStatus( Status );
    void updateStatus();
    static bool freeSpace( const QString &path = QString::null, int min = -1);

    static QString errorMessage( const QString &prefix = QString::null );
    static QString tempDir();
    static void cleanupTempFiles();

private:
    QTemporaryFile *tmpFile;
    QTextStream *ts;
    QString lastLine;
    qint64 pos;
    QChar c;
    int len;
    Status mStatus;
    static const unsigned long long minFree = 1024*100;
};
#endif
