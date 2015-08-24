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

#ifndef IMAPTRANSPORT_H
#define IMAPTRANSPORT_H

#include <qstring.h>
#include <qmailtransport.h>

#ifndef QT_NO_SSL
#include <QSslError>
#endif

class Rfc1951Compressor;
class Rfc1951Decompressor;

//TODO: Make the imap* functions in the base class virtual. Binary incompatible change (BIC)
class ImapTransport : public QMailTransport
{
    Q_OBJECT

public:
    ImapTransport(const char* name);
    ~ImapTransport();

    // Read line-oriented data from the transport (must have an open connection)
    bool imapCanReadLine();
    bool imapBytesAvailable();
    QByteArray imapReadLine();
    QByteArray imapReadAll();

    // Write data to the transport (must have an open connection)
    bool imapWrite(QByteArray *in);

    // Set/Get RFC1951 compression state
    void setCompress(bool comp);
    bool compress();

    void imapClose();

#ifndef QT_NO_SSL
protected:
    virtual bool ignoreCertificateErrors(const QList<QSslError>& errors);
#endif

private:
    void test();

private:
    bool _compress;
    Rfc1951Decompressor *_decompressor;
    Rfc1951Compressor *_compressor;
};

#endif
