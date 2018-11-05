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

#include "qgpgme.h"

#include <QCryptographicHash>

QMailCryptoGPGME::QMailCryptoGPGME(gpgme_protocol_t protocol): m_protocol(protocol), m_cb(0)
{
    gpgme_error_t err;

    gpgme_check_version(NULL);
    err = gpgme_engine_check_version(protocol);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << QStringLiteral("cannot use %1 engine.").arg(gpgme_get_protocol_name(protocol));

        gpgme_engine_info_t info;
        err = gpgme_get_engine_info(&info);
        if (!err) {
            while (info) {
                qWarning() << "protocol:" << gpgme_get_protocol_name(info->protocol);
                if (info->file_name && !info->version)
                    qWarning() << "engine " << info->file_name << " not installed properly";
                else if (info->file_name && info->version && info->req_version)
                    qWarning() << "engine " << info->file_name
                               << " version " << info->version
                               << " installed, but at least version "
                               << info->req_version << " required";
                else
                    qWarning() << "unknown issue";
                info = info->next;
            }
        } else {
            qWarning() << "cannot get engine info:" << gpgme_strerror(err);
        }
    }
}

static QByteArray canonicalizeStr(const char *str)
{
    const char *p;

    QByteArray out;
    out.reserve(strlen(str) * 1.1);
    for (p = str; *p != '\0'; ++p) {
        if (*p != '\r') {
            if (*p == '\n')
                out.append('\r');
            out.append(*p);
        }
    }

    return out;
}

static QMailCryptoFwd::SignatureResult toSignatureResult(gpgme_error_t err)
{
    switch (gpgme_err_code(err)) {
    case GPG_ERR_NO_ERROR:
        return QMailCryptoFwd::SignatureValid;
    case GPG_ERR_SIG_EXPIRED:
        return QMailCryptoFwd::SignatureExpired;
    case GPG_ERR_KEY_EXPIRED:
        return QMailCryptoFwd::KeyExpired;
    case GPG_ERR_CERT_REVOKED:
        return QMailCryptoFwd::CertificateRevoked;
    case GPG_ERR_UNUSABLE_SECKEY:
        return QMailCryptoFwd::UnusableKey;
    case GPG_ERR_BAD_PASSPHRASE:
        return QMailCryptoFwd::BadPassphrase;
    case GPG_ERR_BAD_SIGNATURE:
        return QMailCryptoFwd::BadSignature;
    case GPG_ERR_NO_DATA:
    case GPG_ERR_INV_VALUE:
    case GPG_ERR_NO_PUBKEY:
        return QMailCryptoFwd::MissingKey;
    default:
        return QMailCryptoFwd::UnknownError;
    }
}

void QMailCryptoGPGME::setPassphraseCallback(QMailCryptoFwd::PassphraseCallback cb)
{
    m_cb = cb;
}

QString QMailCryptoGPGME::passphraseCallback(const QString &info) const
{
    if (m_cb)
        return m_cb(info);
    return QString();
}

// To be used later, see the if (m_cb) in getSignature.
// static gpgme_error_t gpgme_passphraseCallback(void *hook, const char *uid_hint,
//                                               const char *passphrase_info,
//                                               int prev_was_bad, int fd)
// {
//     QMailCryptoSMIME *obj = static_cast<QMailCryptoSMIME*>(hook);

//     (void)uid_hint;
//     (void)prev_was_bad;

//     QString pass = obj->passphraseCallback(QString(passphrase_info));
//     gpgme_io_write(fd, pass.toLocal8Bit().data(), pass.length());
//     gpgme_io_write(fd, "\n", 1);

//     return GPG_ERR_NO_ERROR;
// }

struct GPGmeContext {
    gpgme_ctx_t ctx;
    gpgme_error_t err;
    GPGmeContext(gpgme_protocol_t protocol)
        : ctx(0), err(0)
    {
        err = gpgme_new(&ctx);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
            ctx = 0;
            return;
        }
        err = gpgme_set_protocol(ctx, protocol);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
            gpgme_release(ctx);
            ctx = 0;
            return;
        }
    }
    ~GPGmeContext()
    {
        if (ctx) {
            gpgme_release(ctx);
        }
    }
    bool operator!()
    {
        return (ctx == 0);
    }
    operator gpgme_ctx_t() const
    {
        return ctx;
    }
    QByteArray errorMessage() const
    {
        return (err && gpgme_err_code(err) != GPG_ERR_NO_ERROR)
            ? QByteArray(gpgme_strerror(err)) : QByteArray();
    }
};

struct GPGmeData {
    gpgme_data_t data;
    gpgme_error_t err;
    GPGmeData()
        : data(0), err(0)
    {
        err = gpgme_data_new(&data);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR)
            data = 0;
    }
    // The data hold by origin are not copied. origin should be valid
    // for the whole life of the created structure.
    GPGmeData(const QByteArray &origin)
        : data(0), err(0)
    {
        const int NO_COPY = 0;
        err = gpgme_data_new_from_mem(&data, origin.constData(),
                                      origin.length(), NO_COPY);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR)
            data = 0;
    }
    ~GPGmeData()
    {
        if (data)
            gpgme_data_release(data);
    }
    operator gpgme_data_t() const
    {
        return data;
    }
    bool operator!()
    {
        return (data == 0);
    }
    QByteArray errorMessage() const
    {
        return (err && gpgme_err_code(err) != GPG_ERR_NO_ERROR)
            ? QByteArray(gpgme_strerror(err)) : QByteArray();
    }
    QByteArray releaseData()
    {
        size_t ln;
        char *signData = gpgme_data_release_and_get_mem(data, &ln);
        data = 0;

        QByteArray output(signData, ln);
        gpgme_free(signData);

        return output;
    }
};

QMailCryptoFwd::SignatureResult QMailCryptoGPGME::getSignature(const QByteArray &message,
                                                               const QStringList &keys,
                                                               QByteArray &result,
                                                               QByteArray &micalg) const
{
    result.clear();
    micalg.clear();

    GPGmeData sig;
    if (!sig) {
        qWarning() << "cannot create signature data:" << sig.errorMessage();
        return toSignatureResult(sig.err);
    }
    gpgme_error_t err;
    err = gpgme_data_set_encoding(sig, GPGME_DATA_ENCODING_ARMOR);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot set ascii armor on signature:" << gpgme_strerror(err);
        return toSignatureResult(err);
    }

    QByteArray mess(canonicalizeStr(message));
    GPGmeData data(mess);
    if (!data) {
        qWarning() << "cannot create message data:" << data.errorMessage();
        return toSignatureResult(data.err);
    }

    GPGmeContext ctx(m_protocol);
    if (!ctx) {
        qWarning() << "cannot create context:" << ctx.errorMessage();
        return toSignatureResult(ctx.err);
    }
    gpgme_set_textmode(ctx, 1);
    gpgme_set_armor(ctx, 1);
    gpgme_signers_clear(ctx);

    for (QStringList::const_iterator it = keys.begin(); it != keys.end(); it++) {
        gpgme_key_t key;
        const int RETRIEVE_SECRET_KEY = 1;
        err = gpgme_get_key(ctx, it->toLocal8Bit().data(), &key, RETRIEVE_SECRET_KEY);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
            qWarning() << "cannot retrieve key" << *it;
            return toSignatureResult(err);
        }
        err = gpgme_signers_add(ctx, key);
        if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
            qWarning() << "cannot add key" << *it;
            gpgme_key_unref(key);
            return toSignatureResult(err);
        }
        gpgme_key_unref(key);
    }

    if (m_cb) {
        // Only working for gnupg >= 2.1
        // Will be activated later.
        // gpgme_set_pinentry_mode(ctx, GPGME_PINENTRY_MODE_LOOPBACK);
        // gpgme_set_passphrase_cb(ctx, gpgme_passphraseCallback, (void*)this);
        qWarning() << "passphrase callback not implemented";

        // For testing purpose, just creating a md5 of data
        // and use this as signature data.
        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData(mess, strlen(mess));
        result = md5.result().toHex();

        micalg = "pgp-md5";

        return QMailCryptoFwd::SignatureValid;
    }

    err = gpgme_op_sign(ctx, data, sig, GPGME_SIG_MODE_DETACH);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot sign" << gpgme_strerror(err);
        return toSignatureResult(err);
    }

    gpgme_sign_result_t res;
    res = gpgme_op_sign_result(ctx);
    if (res->invalid_signers) {
        qWarning() << "found invalid signer" << res->invalid_signers->fpr;
        return QMailCryptoFwd::MissingKey;
    }
    if (!res->signatures || res->signatures->next) {
        qWarning() << "found zero or more than one signature";
        return QMailCryptoFwd::MissingKey;
    }

    micalg = "pgp-";
    micalg += QByteArray(gpgme_hash_algo_name(res->signatures->hash_algo)).toLower();

    result = sig.releaseData();

    return QMailCryptoFwd::SignatureValid;
}

QMailCryptoFwd::SignatureResult QMailCryptoGPGME::computeSignature(QMailMessagePartContainer &part,
                                                                   const QStringList &keys,
                                                                   QByteArray &signedData,
                                                                   QByteArray &micalg) const
{
    /* Generate the transfer output corresponding to part. */
    QMailMessagePart data;
    if (!partHasSignature(part)) {
        data.setMultipartType(part.multipartType());
        if (part.multipartType() == QMailMessagePartContainerFwd::MultipartNone) {
            data.setBody(part.body());
        } else {
            for (uint i = 0; i < part.partCount(); i++)
                data.appendPart(part.partAt(i));
        }
    } else {
        data = part.partAt(0);
    }
    QByteArray result = data.toRfc2822();

    QMailCryptoFwd::SignatureResult out;
    out = getSignature(result, keys, signedData, micalg);
    if (out != QMailCryptoFwd::SignatureValid)
        return out;

    /* Signature data has been successfully generated. */
    data.setUndecodedData(result);

    // Change the part object to have two parts, if not already.
    if (!partHasSignature(part)) {
        if (part.multipartType() != QMailMessagePartContainerFwd::MultipartNone) {
            // Erase content.
            part.clearParts();
        }
        // Setup new two parts content.
        part.appendPart(data);
        QMailMessagePart signature;
        part.appendPart(signature);
    }

    return QMailCryptoFwd::SignatureValid;
}

QMailCryptoFwd::SignatureResult QMailCryptoGPGME::verify(const QByteArray &sigData,
                                                         const QByteArray &messageData,
                                                         QList<QMailCryptoFwd::KeyResult> &keyResults) const
{
    keyResults.clear();

    GPGmeData sig(sigData);
    if (!sig) {
        qWarning() << "cannot create message data:" << sig.errorMessage();
        return toSignatureResult(sig.err);
    }

    QByteArray mess(canonicalizeStr(messageData.constData()));
    GPGmeData signed_text(mess);
    if (!signed_text) {
        qWarning() << "cannot create signature data:" << signed_text.errorMessage();
        return toSignatureResult(signed_text.err);
    }

    GPGmeContext ctx(m_protocol);
    if (!ctx) {
        qWarning() << "cannot create context:" << ctx.errorMessage();
        return toSignatureResult(ctx.err);
    }

    gpgme_error_t err;
    err = gpgme_op_verify(ctx, sig, signed_text, NULL);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "verification fails:" << gpgme_strerror(err);
        return toSignatureResult(err);
    }

    gpgme_verify_result_t verif = gpgme_op_verify_result(ctx);
    if (!verif) {
        return QMailCryptoFwd::UnknownError;
    }

    err = GPG_ERR_NO_ERROR;
    gpgme_signature_t signature = verif->signatures;
    while (signature) {
        gpgme_error_t sigErr = signature->status;
        QVariantMap details;
        details.insert("creation date", QVariant(QDateTime::fromTime_t(signature->timestamp)));
        if (signature->exp_timestamp)
            details.insert("expiration date", QVariant(QDateTime::fromTime_t(signature->exp_timestamp)));
        keyResults.append(QMailCryptoFwd::KeyResult(signature->fpr,
                                                    toSignatureResult(sigErr),
                                                    details));
        if (gpgme_err_code(sigErr) != GPG_ERR_NO_ERROR)
            err = signature->status;
        signature = signature->next;
    }

    return toSignatureResult(err);
}
