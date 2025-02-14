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

#ifndef SMIMEPLUGIN_H
#define SMIMEPLUGIN_H

#include "qgpgme.h"

class QMailCryptoSMIME : public QMailCryptoGPGME
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QMailCryptographicServiceInterface")

public:
    QMailCryptoSMIME();
    ~QMailCryptoSMIME() {}

    bool partHasSignature(const QMailMessagePartContainer &part) const Q_DECL_OVERRIDE;
    QMailCryptoFwd::VerificationResult verifySignature(const QMailMessagePartContainer &part) const Q_DECL_OVERRIDE;
    QMailCryptoFwd::SignatureResult sign(QMailMessagePartContainer *part,
                                         const QStringList &keys) const Q_DECL_OVERRIDE;

    bool canDecrypt(const QMailMessagePartContainer &part) const Q_DECL_OVERRIDE;
    QMailCryptoFwd::DecryptionResult decrypt(QMailMessagePartContainer *part) const Q_DECL_OVERRIDE;
};

#endif
