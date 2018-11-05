/* -*- c-basic-offset: 4 -*- */
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

#include <QtTest>
#include <QObject>
#include <QFile>

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include <gpgme.h>

#include <qmailmessage.h>
#include <qmailcontentmanager.h>
#include <qmailcrypto.h>

Q_DECLARE_METATYPE(QMailCryptoFwd::SignatureResult)

class tst_Crypto : public QObject
{
    Q_OBJECT

public:
    tst_Crypto();
    virtual ~tst_Crypto();

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void extractUndecodedData_data();
    void extractUndecodedData();
    void verify_data();
    void verify();
    void sign_data();
    void sign();
    void storage_data();
    void storage();
    void signVerify();

private:
    void importKey(const QString &path, gpgme_protocol_t protocol, QString *storing);
    void deleteKey(const QString &fingerprint, gpgme_protocol_t protocol);
    QString m_pgpKey, m_smimeKey;
};

tst_Crypto::tst_Crypto()
{
    gpgme_error_t err;

    gpgme_check_version(NULL);
    err = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot use OpenPGP engine" << gpgme_strerror(err);

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

tst_Crypto::~tst_Crypto()
{
}

void tst_Crypto::importKey(const QString &path, gpgme_protocol_t protocol,
                           QString *storing)
{
    gpgme_error_t err;
    gpgme_ctx_t ctx;
    gpgme_data_t key;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QFAIL("Key file does not exist or cannot be opened for reading!");
    }

    err = gpgme_new(&ctx);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot create context" << gpgme_strerror(err);
        f.close();
        return;
    }

    err = gpgme_set_protocol(ctx, protocol);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << QStringLiteral("cannot use %1 engine.").arg(gpgme_get_protocol_name(protocol));
        f.close();
        gpgme_release(ctx);
        return;
    }

    err = gpgme_data_new_from_fd(&key, f.handle());
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot create key data" << gpgme_strerror(err);
        f.close();
        gpgme_release(ctx);
        return;
    }

    err = gpgme_op_import(ctx, key);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot import key data" << gpgme_strerror(err);
    }

    gpgme_import_result_t result;
    result = gpgme_op_import_result(ctx);
    if (result->imports && storing)
        *storing = result->imports->fpr;

    gpgme_data_release(key);
    f.close();
    gpgme_release(ctx);
}

void tst_Crypto::deleteKey(const QString &fingerprint, gpgme_protocol_t protocol)
{
    gpgme_error_t err;
    gpgme_ctx_t ctx;
    gpgme_key_t key;

    if (fingerprint.isEmpty())
        return;

    err = gpgme_new(&ctx);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot create context" << gpgme_strerror(err);
        return;
    }
    err = gpgme_set_protocol(ctx, protocol);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << QStringLiteral("cannot use %1 engine.").arg(gpgme_get_protocol_name(protocol));
        gpgme_release(ctx);
        return;
    }

    err = gpgme_get_key(ctx, fingerprint.toLocal8Bit().data(), &key, 1);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot retrieve key" << fingerprint;
        gpgme_release(ctx);
        return;
    }
    err = gpgme_op_delete(ctx, key, 1);
    if (gpgme_err_code(err) != GPG_ERR_NO_ERROR) {
        qWarning() << "cannot delete key" << gpgme_strerror(err);
    }
    gpgme_key_unref(key);

    gpgme_release(ctx);
}

static QString passphrase(const QString &info)
{
    Q_UNUSED(info);

    return QString("test");
}

void tst_Crypto::initTestCase()
{
    importKey(QStringLiteral("%1/testdata/key.asc").arg(QCoreApplication::applicationDirPath()), GPGME_PROTOCOL_OpenPGP, &m_pgpKey); // no pass
    importKey(QStringLiteral("%1/testdata/QMFtest.pem").arg(QCoreApplication::applicationDirPath()), GPGME_PROTOCOL_CMS, &m_smimeKey); // pass for it is QMFtest2018
    QFile::copy(QStringLiteral("%1/testdata/FECA2AF719090DD594C02C27F9CB3F8ED7EDAB31.key").arg(QCoreApplication::applicationDirPath()),
                QDir::homePath() + QDir::separator() + ".gnupg/private-keys-v1.d/FECA2AF719090DD594C02C27F9CB3F8ED7EDAB31.key");

    QMailAccount account;
    account.setName("Account 1");
    account.setMessageType(QMailMessage::Email);
    account.setCustomField("verified", "true");
    QMailAccountConfiguration config;
    QMailStore::instance()->addAccount(&account, &config);
}

void tst_Crypto::cleanupTestCase()
{
    deleteKey(m_pgpKey, GPGME_PROTOCOL_OpenPGP);
    QMailStore::instance()->removeAccounts(QMailAccountKey::customField("verified"));
}

void tst_Crypto::init()
{
}

void tst_Crypto::cleanup()
{
}

// RFC 2822 messages use CRLF as the newline indicator
#define CRLF "\015\012"
#define CR "\015"
#define LF "\012"

void tst_Crypto::extractUndecodedData_data()
{
    QTest::addColumn<QByteArray>("boundary");
    QTest::addColumn<QByteArray>("body");

    QTest::newRow("empty part")
        << QByteArray(CRLF "--_d69d3a43-654c-46a3-ae21-a1fd4e0036a1_" CRLF)
        << QByteArray(CRLF);

    QTest::newRow("part with LF")
        << QByteArray(LF "--_d69d3a43-654c-46a3-ae21-a1fd4e0036a1_" LF)
        << QByteArray("Content-Type: text/plain; charset=UTF-8" LF
                      "Content-Transfer-Encoding: base64" LF
                      LF
                      "SSdtIHN1cmUgdGhlIHRleHQgaXMgcHJpc3RpbmUuIA==");

    QTest::newRow("part with CRLF")
        << QByteArray(CRLF "--_d69d3a43-654c-46a3-ae21-a1fd4e0036a1_" CRLF)
        << QByteArray("Content-Type: text/plain; charset=UTF-8" CRLF
                      "Content-Transfer-Encoding: base64" CRLF
                      CRLF
                      "SSdtIHN1cmUgdGhlIHRleHQgaXMgcHJpc3RpbmUuIA==");
}

void tst_Crypto::extractUndecodedData()
{
    QByteArray headers("Subject: Testing signing" CRLF
                       "Content-Type: multipart/signed; micalg=pgp-sha1;" CRLF
                       " protocol=application/pgp-signature;" CRLF
                       " boundary=_d69d3a43-654c-46a3-ae21-a1fd4e0036a1_" CRLF
                       "MIME-Version: 1.0" CRLF);
    QByteArray footer("Content-Type: application/pgp-signature" CRLF
                      "Content-Transfer-Encoding: 7bit" CRLF
                      "Content-Description: OpenPGP digital signature" CRLF
                      CRLF
                      "abc" CRLF
                      "--_d69d3a43-654c-46a3-ae21-a1fd4e0036a1_--");

    QFETCH(QByteArray, boundary);
    QFETCH(QByteArray, body);

    QMailMessage mail(QMailMessage::fromRfc2822(headers + boundary + body + boundary + footer));
    QCOMPARE(int(mail.partCount()), 2);
    const QMailMessagePart part = mail.partAt(0);
    QCOMPARE(part.undecodedData(), body);
}

void tst_Crypto::verify_data()
{
    QTest::addColumn<QString>("rfc2822Filename");
    QTest::addColumn<QMailCryptoFwd::SignatureResult>("expectedStatus");
    QTest::addColumn<QString>("expectedKeyId");

    QTest::newRow("no multipart/signed mail")
        << QStringLiteral("testdata/nosig")
        << QMailCryptoFwd::MissingSignature
        << QString();
    QTest::newRow("missing key")
        << QStringLiteral("testdata/nokey")
        << QMailCryptoFwd::MissingKey
        << QStringLiteral("FF914AF0C2B35520");
    QTest::newRow("wrong signature data")
        << QStringLiteral("testdata/badsigdata")
        << QMailCryptoFwd::BadSignature
        << QStringLiteral("B2D77D1683AF79EC");
    QTest::newRow("valid signature mail")
        << QStringLiteral("testdata/validsig")
        << QMailCryptoFwd::SignatureValid
        << QStringLiteral("A1B8EF3A16C0B81ECF216C33B2D77D1683AF79EC");
    QTest::newRow("valid S/MIME mail")
        << QStringLiteral("testdata/smimesig")
        << QMailCryptoFwd::SignatureValid
        << QStringLiteral("8ECF3CB1EBE13E05407F11231FC6DFAC3A126948");
}

void tst_Crypto::verify()
{
    QFETCH(QString, rfc2822Filename);
    QFETCH(QMailCryptoFwd::SignatureResult, expectedStatus);
    QFETCH(QString, expectedKeyId);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(),
                                        rfc2822Filename));
    if (!f.open(QIODevice::ReadOnly)) {
        QFAIL("Mail file does not exist or cannot be opened for reading!");
    }

    QMailMessage msg = QMailMessage::fromRfc2822File(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(),
                                        rfc2822Filename));

    QMailCryptoFwd::VerificationResult result = QMailCryptographicServiceFactory::verifySignature(msg);
    QCOMPARE(result.summary, expectedStatus);
    if (expectedStatus == QMailCryptoFwd::MissingSignature
        || expectedStatus == QMailCryptoFwd::UnknownError)
        return;
    QCOMPARE(result.keyResults.length(), 1);
    QCOMPARE(result.keyResults.at(0).key, expectedKeyId);
    QCOMPARE(result.keyResults.at(0).status, expectedStatus);
}

void tst_Crypto::sign_data()
{
    QTest::addColumn<QString>("rfc2822Filename");
    QTest::addColumn<QString>("plugin");
    QTest::addColumn<QString>("fingerprint");
    QTest::addColumn<QMailCryptoFwd::SignatureResult>("expectedStatus");
    QTest::addColumn<QString>("signedFilename");

    QTest::newRow("sign multipart/none mail with OpenPGP")
        << QStringLiteral("testdata/nosig")
        << QStringLiteral("libgpgme.so")
        << m_pgpKey
        << QMailCryptoFwd::SignatureValid
        << QStringLiteral("testdata/aftersig");

    QTest::newRow("sign multipart/none mail with S/MIME")
        << QStringLiteral("testdata/nosig")
        << QStringLiteral("libsmime.so")
        << m_smimeKey
        << QMailCryptoFwd::SignatureValid
        << QStringLiteral("testdata/aftersmime");
}

void tst_Crypto::sign()
{
    QFETCH(QString, rfc2822Filename);
    QFETCH(QString, plugin);
    QFETCH(QString, fingerprint);
    QFETCH(QMailCryptoFwd::SignatureResult, expectedStatus);
    QFETCH(QString, signedFilename);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(),
                                        rfc2822Filename));
    if (!f.open(QIODevice::ReadOnly)) {
        QFAIL("Mail file does not exist or cannot be opened for reading!");
    }

    // Check message signing.
    QMailMessage msg = QMailMessage::fromRfc2822File(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(),
                                                                                 rfc2822Filename));
    QCOMPARE(QMailCryptographicServiceFactory::sign(msg, plugin, QStringList(fingerprint), passphrase), expectedStatus);

    // Check signed message output.
    // Replace the random boundary strings with a fixed one for comparison.
    QRegExp rx("qmf:[^=]+==");
    QString signedMsg = QString(msg.toRfc2822());
    signedMsg.replace(rx, "testingtestingtesting");
    QFile f2(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(),
                                         signedFilename));
    if (!f2.open(QIODevice::ReadOnly)) {
        QFAIL("Mail file does not exist or cannot be opened for reading!");
    }
    QCOMPARE(signedMsg, QString(f2.readAll()));

    // Check that signature is valid.
    // To be activated later when the passphrase callback will be working
    // with gnupg >= 2.1
    // QCOMPARE(QMailCryptographicServiceFactory::verifySignature(msg), expectedStatus);
}

void tst_Crypto::signVerify()
{
    // Create a message.
    QMailMessage message;
    message.setMessageType(QMailMessage::Email);
    QMailMessageContentType type("text/plain; charset=UTF-8");
    message.setBody(QMailMessageBody::fromData("test", type, QMailMessageBody::Base64));

    // Sign it with the PGP key (no password).
    QMailCryptoFwd::SignatureResult r = QMailCryptographicServiceFactory::sign(message, "libgpgme.so", QStringList() << m_pgpKey);
    QCOMPARE(r, QMailCryptoFwd::SignatureValid);
    QCOMPARE(message.partCount(), uint(2));
    QCOMPARE(message.contentType().type(), QByteArray("multipart"));
    QCOMPARE(message.contentType().subType(), QByteArray("signed"));

    // And verify it.
    QMailCryptoFwd::VerificationResult v = QMailCryptographicServiceFactory::verifySignature(message);
    QCOMPARE(v.summary, QMailCryptoFwd::SignatureValid);
    QCOMPARE(v.engine, QStringLiteral("libgpgme.so"));
    QCOMPARE(v.keyResults.length(), 1);
    QCOMPARE(v.keyResults[0].key, m_pgpKey);
    QCOMPARE(v.keyResults[0].status, QMailCryptoFwd::SignatureValid);
}

void tst_Crypto::storage_data()
{
    QTest::addColumn<QString>("rfc2822Filename");
    QTest::addColumn<QString>("plugin");

    QTest::newRow("multipart/signed mail")
        << QStringLiteral("testdata/validsig")
        << QStringLiteral("qmfstoragemanager");
}

void tst_Crypto::storage()
{
    QFETCH(QString, rfc2822Filename);
    QFETCH(QString, plugin);

    QFile f(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(),
                                        rfc2822Filename));
    if (!f.open(QIODevice::ReadOnly)) {
        QFAIL("Mail file does not exist or cannot be opened for reading!");
    }

    // Store message.
    QMailMessage msg = QMailMessage::fromRfc2822File(QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(),
                                                                                 rfc2822Filename));
    QCOMPARE(msg.partCount(), uint(2));
    QVERIFY(!msg.partAt(0).undecodedData().isEmpty());

    QMailContentManager *store = QMailContentManagerFactory::create(plugin);
    QVERIFY(store);
    QCOMPARE(store->add(&msg, QMailContentManager::EnsureDurability),
             QMailStore::NoError);
    QVERIFY(QFile(msg.contentIdentifier() + "-parts/1-raw").exists());

    QMailMessage msg2;
    QCOMPARE(store->load(msg.contentIdentifier(), &msg2), QMailStore::NoError);
    QCOMPARE(msg.partAt(0).undecodedData(), msg2.partAt(0).undecodedData());
}

#include "tst_crypto.moc"
QTEST_MAIN(tst_Crypto)
