/****************************************************************************
**
** Copyright (C) 2018 Caliste Damien.
** Contact: Damien Caliste <dcaliste@free.fr>
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#ifndef QMAILCRYPTOGPGME_H
#define QMAILCRYPTOGPGME_H

#include "qmailcrypto.h"

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include <gpgme.h>

#include <QObject>

class QMailCryptoGPGME: public QObject,
    public QMailCryptographicServiceInterface
{
    Q_OBJECT
    Q_INTERFACES(QMailCryptographicServiceInterface)

 public:
    ~QMailCryptoGPGME() {}

    void setPassphraseCallback(QMailCryptoFwd::PassphraseCallback cb) Q_DECL_OVERRIDE;
    QString passphraseCallback(const QString &info) const Q_DECL_OVERRIDE;

 protected:
    QMailCryptoGPGME(gpgme_protocol_t protocol);

    QMailCryptoFwd::SignatureResult computeSignature(QMailMessagePartContainer &part,
                                                     const QStringList &keys,
                                                     QByteArray &signedData,
                                                     QByteArray &micalg) const;
    QMailCryptoFwd::SignatureResult verify(const QByteArray &sigData,
                                           const QByteArray &messageData,
                                           QList<QMailCryptoFwd::KeyResult> &keyResults) const;

    QMailCryptoFwd::CryptResult decrypt(const QByteArray &encData,
                                        QByteArray &decData) const;

 private:
    QMailCryptoFwd::SignatureResult getSignature(const QByteArray &message,
                                                 const QStringList &keys,
                                                 QByteArray &result,
                                                 QByteArray &micalg) const;

    gpgme_protocol_t m_protocol;
    QMailCryptoFwd::PassphraseCallback m_cb;
};

#endif
