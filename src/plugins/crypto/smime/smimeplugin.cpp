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

#include "smimeplugin.h"
#include <qmaillog.h>

QMailCryptoSMIME::QMailCryptoSMIME() : QMailCryptoGPGME(GPGME_PROTOCOL_CMS)
{
}

bool QMailCryptoSMIME::partHasSignature(const QMailMessagePartContainer &part) const
{
    if (part.multipartType() != QMailMessagePartContainer::MultipartSigned || part.partCount() != 2)
        return false;

    const QMailMessagePart signature = part.partAt(1);

    return signature.contentType().matches("application", "pkcs7-signature")
           || signature.contentType().matches("application", "x-pkcs7-signature");
}

QMailCrypto::VerificationResult QMailCryptoSMIME::verifySignature(const QMailMessagePartContainer &part) const
{
    if (!partHasSignature(part))
        return QMailCrypto::VerificationResult(QMailCrypto::MissingSignature);

    QMailMessagePart body = part.partAt(0);
    QMailMessagePart signature = part.partAt(1);

    if (!body.contentAvailable() || !signature.contentAvailable())
        return QMailCrypto::VerificationResult();

    QMailCrypto::VerificationResult result;
    result.engine = QStringLiteral("libsmime.so");
    result.summary = verify(signature.body().data(QMailMessageBody::Decoded),
                            body.undecodedData(), result.keyResults);
    return result;
}

QMailCrypto::SignatureResult QMailCryptoSMIME::sign(QMailMessagePartContainer *part,
                                                       const QStringList &keys) const
{
    if (!part) {
        qCWarning(lcMessaging) << "unable to sign a NULL part.";
        return QMailCrypto::UnknownError;
    }

    QByteArray signedData, micalg;
    QMailCrypto::SignatureResult result;
    result = computeSignature(*part, keys, signedData, micalg);
    if (result != QMailCrypto::SignatureValid)
        return result;

    // Set it to multipart/signed content-type.
    QList<QMailMessageHeaderField::ParameterType> parameters;
    parameters << QMailMessageHeaderField::ParameterType("micalg", micalg);
    parameters << QMailMessageHeaderField::ParameterType("protocol", "application/pkcs7-signature");
    part->setMultipartType(QMailMessagePartContainer::MultipartSigned, parameters);

    // Write the signature data in the second part.
    QMailMessagePart &signature = part->partAt(1);
    QMailMessageContentDisposition disposition;
    disposition.setType(QMailMessageContentDisposition::Attachment);
    disposition.setFilename("smime.p7s");
    signature.setContentDisposition(disposition);
    signature.setBody(QMailMessageBody::fromData(signedData,
                                                 QMailMessageContentType("application/pkcs7-signature"),
                                                 QMailMessageBody::Base64));

    return QMailCrypto::SignatureValid;
}

bool QMailCryptoSMIME::canDecrypt(const QMailMessagePartContainer &part) const
{
    Q_UNUSED(part);
    return false;
}

QMailCrypto::DecryptionResult QMailCryptoSMIME::decrypt(QMailMessagePartContainer *part) const
{
    Q_UNUSED(part);
    return QMailCrypto::DecryptionResult(QMailCrypto::UnsupportedProtocol);
}
