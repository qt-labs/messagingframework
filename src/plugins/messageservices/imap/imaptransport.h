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

#ifndef IMAPTRANSPORT_H
#define IMAPTRANSPORT_H

#include <qstring.h>
#include <qmailtransport.h>

#ifndef QT_NO_OPENSSL
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
    QByteArray imapReadLine();

    // Write data to the transport (must have an open connection)
    bool imapWrite(QByteArray *in);

    // Set/Get RFC1951 compression state
    void setCompress(bool comp);
    bool compress();

    void imapClose();

#ifndef QT_NO_OPENSSL
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
