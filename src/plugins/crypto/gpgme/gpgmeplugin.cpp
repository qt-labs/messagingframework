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

#include "gpgmeplugin.h"

QMailCryptoGPG::QMailCryptoGPG() : QMailCryptoGPGME(GPGME_PROTOCOL_OpenPGP)
{
}

bool QMailCryptoGPG::partHasSignature(const QMailMessagePartContainer &part) const
{
    if (part.multipartType() != QMailMessagePartContainerFwd::MultipartSigned ||
        part.partCount() != 2)
        return false;

    const QMailMessagePart signature = part.partAt(1);

    if (!signature.contentType().matches("application", "pgp-signature"))
        return false;

    return true;
}

QMailCryptoFwd::VerificationResult QMailCryptoGPG::verifySignature(const QMailMessagePartContainer &part) const
{
    if (!partHasSignature(part))
        return QMailCryptoFwd::VerificationResult(QMailCryptoFwd::MissingSignature);

    QMailMessagePart body = part.partAt(0);
    QMailMessagePart signature = part.partAt(1);

    if (!body.contentAvailable() ||
        !signature.contentAvailable())
        return QMailCryptoFwd::VerificationResult();

    QMailCryptoFwd::VerificationResult result;
    result.engine = QStringLiteral("libgpgme.so");
    result.summary = verify(signature.body().data(QMailMessageBodyFwd::Decoded),
                      body.undecodedData(), result.keyResults);
    return result;
}

QMailCryptoFwd::SignatureResult QMailCryptoGPG::sign(QMailMessagePartContainer &part,
                                                     const QStringList &keys) const
{
    QByteArray signedData, micalg;
    QMailCryptoFwd::SignatureResult result;
    result = computeSignature(part, keys, signedData, micalg);
    if (result != QMailCryptoFwd::SignatureValid)
        return result;

    // Set it to multipart/signed content-type.
    QList<QMailMessageHeaderField::ParameterType> parameters;
    parameters << QMailMessageHeaderField::ParameterType("micalg", micalg);
    parameters << QMailMessageHeaderField::ParameterType("protocol", "application/pgp-signature");
    part.setMultipartType(QMailMessagePartContainerFwd::MultipartSigned, parameters);

    // Write the signature data in the second part.
    QMailMessagePart &signature = part.partAt(1);

    signature.setBody(QMailMessageBody::fromData(signedData,
                                                 QMailMessageContentType("application/pgp-signature"),
                                                 QMailMessageBody::SevenBit));
    signature.setContentDescription("OpenPGP digital signature");

    return QMailCryptoFwd::SignatureValid;
}
