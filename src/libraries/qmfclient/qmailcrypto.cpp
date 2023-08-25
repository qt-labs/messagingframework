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

#include "qmailcrypto.h"

#include <QCoreApplication>

struct SignedContainerFinder
{
    QMailCryptographicServiceInterface *m_engine;
    QMailMessagePartContainer *m_signedContainer;
    const QMailMessagePartContainer *m_signedConstContainer;

    SignedContainerFinder(QMailCryptographicServiceInterface *engine)
        : m_engine(engine)
        , m_signedContainer(0)
        , m_signedConstContainer(0)
    {
    }

    bool operator()(QMailMessagePart &part)
    {
        if (m_engine->partHasSignature(part)) {
            m_signedContainer = &part;
            return false; // stop iterating.
        }
        return true; // continue to search.
    }
    bool operator()(const QMailMessagePart &part)
    {
        if (m_engine->partHasSignature(part)) {
            m_signedConstContainer = &part;
            return false; // stop iterating.
        }
        return true; // continue to search.
    }
};

QMailMessagePartContainer* QMailCryptographicServiceInterface::findSignedContainer(QMailMessagePartContainer *part)
{
    if (partHasSignature(*part))
        return part;

    SignedContainerFinder finder(this);
    part->foreachPart<SignedContainerFinder&>(finder);
    return finder.m_signedContainer;
}

const QMailMessagePartContainer* QMailCryptographicServiceInterface::findSignedContainer(const QMailMessagePartContainer *part)
{
    if (partHasSignature(*part))
        return part;

    SignedContainerFinder finder(this);
    part->foreachPart<SignedContainerFinder&>(finder);
    return finder.m_signedConstContainer;
}

QMailCryptographicService* QMailCryptographicService::m_pInstance = 0;

QMailCryptographicService::QMailCryptographicService(QObject* parent)
        : QMailPluginManager(QString::fromLatin1("crypto"), parent)
{
}

QMailCryptographicService::~QMailCryptographicService()
{
}

QMailCryptographicService* QMailCryptographicService::instance()
{
    if (!m_pInstance)
        m_pInstance = new QMailCryptographicService(QCoreApplication::instance());

    return m_pInstance;
}

QMailCryptographicServiceInterface* QMailCryptographicService::instance(const QString &engine)
{
    return qobject_cast<QMailCryptographicServiceInterface*>(QMailPluginManager::instance(engine));
}

QMailCryptographicServiceInterface* QMailCryptographicService::decryptionEngine(const QMailMessagePartContainer &part)
{
    if (part.isEncrypted()) {
        QStringList engines = list();
        for (QStringList::iterator it = engines.begin(); it != engines.end(); it++) {
            QMailCryptographicServiceInterface *engine = instance(*it);
            if (engine && engine->canDecrypt(part))
                return engine;
        }
    }
    return Q_NULLPTR;
}

QMailMessagePartContainer* QMailCryptographicService::findSignedContainer(QMailMessagePartContainer *part, QMailCryptographicServiceInterface **engine)
{
    QMailCryptographicService *plugins =
        QMailCryptographicService::instance();
    QStringList engines = plugins->list();

    if (engine)
        *engine = 0;

    for (QStringList::iterator it = engines.begin(); it != engines.end(); it++) {
        QMailCryptographicServiceInterface *plugin = plugins->instance(*it);
        if (plugin) {
            QMailMessagePartContainer *result = plugin->findSignedContainer(part);
            if (result) {
                if (engine)
                    *engine = plugin;
                return result;
            }
        }
    }

    return 0;
}
const QMailMessagePartContainer* QMailCryptographicService::findSignedContainer(const QMailMessagePartContainer *part, QMailCryptographicServiceInterface **engine)
{
    QMailCryptographicService *plugins =
        QMailCryptographicService::instance();
    QStringList engines = plugins->list();

    if (engine)
        *engine = 0;

    for (QStringList::iterator it = engines.begin(); it != engines.end(); it++) {
        QMailCryptographicServiceInterface *plugin = plugins->instance(*it);
        if (plugin) {
            const QMailMessagePartContainer *result = plugin->findSignedContainer(part);
            if (result) {
                if (engine)
                    *engine = plugin;
                return result;
            }
        }
    }

    return 0;
}

QMailCryptoFwd::VerificationResult QMailCryptographicService::verifySignature(const QMailMessagePartContainer &part)
{
    QMailCryptographicService *plugins =
        QMailCryptographicService::instance();
    QStringList engines = plugins->list();

    for (QStringList::iterator it = engines.begin(); it != engines.end(); it++) {
        QMailCryptographicServiceInterface *engine = plugins->instance(*it);
        if (engine && engine->partHasSignature(part))
            return engine->verifySignature(part);
    }

    return QMailCryptoFwd::VerificationResult(QMailCryptoFwd::MissingSignature);
}

QMailCryptoFwd::SignatureResult QMailCryptographicService::sign(QMailMessagePartContainer *part,
                                                                const QString &crypto,
                                                                const QStringList &keys,
                                                                QMailCryptoFwd::PassphraseCallback cb)
{
    QMailCryptographicService *plugins = QMailCryptographicService::instance();

    QMailCryptographicServiceInterface *engine = plugins->instance(crypto);
    if (engine) {
        engine->setPassphraseCallback(cb);
        return engine->sign(part, keys);
    }

    return QMailCryptoFwd::BadSignature;
}

bool QMailCryptographicService::canDecrypt(const QMailMessagePartContainer &part)
{
    return instance()->decryptionEngine(part) != Q_NULLPTR;
}

QMailCryptoFwd::DecryptionResult QMailCryptographicService::decrypt(QMailMessagePartContainer *part,
                                                                    QMailCryptoFwd::PassphraseCallback cb)
{
    if (!part || !part->isEncrypted()) {
        return QMailCryptoFwd::DecryptionResult(QMailCryptoFwd::NoDigitalEncryption);
    }

    QMailCryptographicServiceInterface *engine = instance()->decryptionEngine(*part);
    if (engine) {
        engine->setPassphraseCallback(cb);
        QMailCryptoFwd::DecryptionResult result = engine->decrypt(part);
        if (result.status == QMailCryptoFwd::Decrypted) {
            QMailMessage* message = dynamic_cast<QMailMessage*>(part);
            if (message && message->hasAttachments()) {
                message->setStatus(QMailMessage::HasAttachments, true);
            }
            if (message && findSignedContainer(part)) {
                message->setStatus(QMailMessage::HasSignature, true);
            }
        }
        return result;
    } else {
        return QMailCryptoFwd::DecryptionResult(QMailCryptoFwd::UnsupportedProtocol);
    }
}
