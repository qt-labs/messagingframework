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

#ifndef QMAILCRYPTO_H
#define QMAILCRYPTO_H

#include "qmailmessage.h"
#include <qmailpluginmanager.h>

class QMailAccountConfiguration;

class QMF_EXPORT QMailCrypto
{
public:
    enum SignatureResult {
        SignatureValid,
        SignatureExpired,
        KeyExpired,
        CertificateRevoked,
        BadPassphrase,
        BadSignature,
        MissingKey,
        MissingSignature,
        UnusableKey,
        UnknownError
    };
    enum CryptResult {
        NoDigitalEncryption,
        Decrypted,
        WrongPassphrase,
        UnsupportedProtocol,
        UnknownCryptError
    };
    typedef QString (*PassphraseCallback)(const QString &info);

    struct QMF_EXPORT KeyResult {
        QString key;
        SignatureResult status;
        QVariantMap details;

        KeyResult(const QString &fingerprint, SignatureResult result,
                  const QVariantMap &parameters = QVariantMap())
            : key(fingerprint), status(result), details(parameters)
        {
        }
    };

    struct QMF_EXPORT VerificationResult {
        QString engine;
        SignatureResult summary;
        QList<KeyResult> keyResults;

        VerificationResult(SignatureResult status = UnknownError,
                           const QList<KeyResult> &results = QList<KeyResult>())
            : summary(status), keyResults(results)
        {
        }
    };

    struct QMF_EXPORT DecryptionResult {
        QString engine;
        CryptResult status;

        DecryptionResult(CryptResult result = UnknownCryptError)
            : status(result)
        {
        }
    };
};

class QMF_EXPORT QMailCryptographicServiceConfiguration
{
public:
    QMailCryptographicServiceConfiguration(QMailAccountConfiguration *config);
    ~QMailCryptographicServiceConfiguration();

    QStringList signatureKeys() const;
    void setSignatureKeys(const QStringList &keys);

    QString signatureType() const;
    void setSignatureType(const QString &method);

    bool useSignatureByDefault() const;
    void setUseSignatureByDefault(bool status);

private:
    class Private;
    Private *d;
};

class QMailCryptographicServiceInterface
{
public:
    virtual ~QMailCryptographicServiceInterface() {}

    virtual bool partHasSignature(const QMailMessagePartContainer &part) const = 0;
    virtual QMailCrypto::VerificationResult verifySignature(const QMailMessagePartContainer &part) const = 0;
    virtual QMailCrypto::SignatureResult sign(QMailMessagePartContainer *part,
                                              const QStringList &keys) const = 0;

    virtual bool canDecrypt(const QMailMessagePartContainer &part) const = 0;
    virtual QMailCrypto::DecryptionResult decrypt(QMailMessagePartContainer *part) const = 0;

    virtual void setPassphraseCallback(QMailCrypto::PassphraseCallback cb) = 0;
    virtual QString passphraseCallback(const QString &info) const = 0;

    QMailMessagePartContainer* findSignedContainer(QMailMessagePartContainer *part);
    const QMailMessagePartContainer* findSignedContainer(const QMailMessagePartContainer *part);
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QMailCryptographicServiceInterface, "org.qt-project.Qt.QMailCryptographicServiceInterface")
QT_END_NAMESPACE

class QMF_EXPORT QMailCryptographicService : public QMailPluginManager
{
    Q_OBJECT

public:
    static QMailCryptographicService* instance();
    static QMailMessagePartContainer* findSignedContainer(QMailMessagePartContainer *part,
                                                          QMailCryptographicServiceInterface **engine = nullptr);
    static const QMailMessagePartContainer* findSignedContainer(const QMailMessagePartContainer *part,
                                                                QMailCryptographicServiceInterface **engine = nullptr);

    QMailCryptographicServiceInterface* instance(const QString &engine);
    QMailCryptographicServiceInterface* decryptionEngine(const QMailMessagePartContainer &part);

    static QMailCrypto::VerificationResult verifySignature(const QMailMessagePartContainer &part);
    static QMailCrypto::SignatureResult sign(QMailMessagePartContainer *part,
                                             const QString &crypto,
                                             const QStringList &keys,
                                             QMailCrypto::PassphraseCallback cb = nullptr);

    static bool canDecrypt(const QMailMessagePartContainer &part);
    static QMailCrypto::DecryptionResult decrypt(QMailMessagePartContainer *part,
                                                 QMailCrypto::PassphraseCallback cb = nullptr);

private:
    QMailCryptographicService(QObject *parent = nullptr);
    ~QMailCryptographicService();

    static QMailCryptographicService* m_pInstance;
};

#endif
