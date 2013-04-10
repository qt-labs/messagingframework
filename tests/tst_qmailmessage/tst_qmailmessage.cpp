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

#include <QObject>
#include <QTest>
#include <qmailaddress.h>
#include <qmailcodec.h>
#include <qmailmessage.h>
#include <qmailtimestamp.h>

/*
Note: Any email addresses appearing in this test data must be example addresses,
as defined by RFC 2606.  Therefore, they should use one of the following domains:
    *.example.{com|org|net}
    *.test
    *.example
*/

// RFC 2822 messages use CRLF as the newline indicator
#define CRLF "\015\012"

//TESTED_CLASS=QMailMessage
//TESTED_FILES=src/libraries/qtopiamail/qmailmessage.cpp

/*
    This class primarily tests that QMailMessage correctly handles e-mail messages.
*/
class tst_QMailMessage : public QObject
{
    Q_OBJECT

public:
    tst_QMailMessage();
    virtual ~tst_QMailMessage();

private slots:
    virtual void initTestCase();
    virtual void cleanupTestCase();
    virtual void init();
    virtual void cleanup();

    void toRfc2822_data();
    void toRfc2822();
    void fromRfc2822_data();
    void fromRfc2822();

    void id();
    void setId();

    void parentFolderId();
    void setParentFolderId();

    void messageType();
    void setMessageType_data();
    void setMessageType();

    void setFrom();
    void from();

    void subject();
    void setSubject();

    void date();
    void setDate();

    void to();
    void setTo_data();
    void setTo();
    void cc();
    void setCc();
    void bcc();
    void setBcc();

    void recipients_data();
    void recipients();
    void hasRecipients();

    void replyTo();
    void setReplyTo();

    void inReplyTo();
    void setInReplyTo();

/*
    void status();
    void setStatus();
*/

    void serverUid();
    void setServerUid();

/*
    void size();
    void setSize();
*/

    void multiMultipart();

    void copyAndAssign();

    void unterminatedDoubleQuote();
};

QTEST_MAIN(tst_QMailMessage)

#include "tst_qmailmessage.moc"

static void testHeader(const QString& name, void (QMailMessage::*setter)(const QString&), QString (QMailMessage::*getter)() const, bool expandEncodedWords = false)
{
    QString value1("This is a string value");
    QString value2("This is a different string value");

    QMailMessage m;
    QCOMPARE((m.*getter)(), QString());
    if (!name.isNull()) {
        QCOMPARE(m.headerFieldText(name), QString());
        QCOMPARE(m.headerFieldsText(name), QStringList());
    }
    QCOMPARE(m.contentModified(), false);

    (m.*setter)(value1);
    QCOMPARE((m.*getter)(), value1);
    if (!name.isNull()) {
        QCOMPARE(m.headerFieldText(name), value1);
        QCOMPARE(m.headerFieldsText(name), QStringList(value1));
        QCOMPARE(m.contentModified(), true);
    } else {
        QCOMPARE(m.dataModified(), true);
    }

    (m.*setter)(value2);
    QCOMPARE((m.*getter)(), value2);
    if (!name.isNull()) {
        QCOMPARE(m.headerFieldText(name), value2);
        QCOMPARE(m.headerFieldsText(name), QStringList(value2));
        QCOMPARE(m.contentModified(), true);
    } else {
        QCOMPARE(m.dataModified(), true);
    }

    if (expandEncodedWords && !name.isNull())
    {
        QString value3("This =?ISO-8859-1?Q?value?= contains =?ISO-8859-1?B?ZW5jb2RlZC13b3Jkcw==?=");
        QString value4("This value contains encoded-words");

        (m.*setter)(value3);
        QCOMPARE((m.*getter)(), value4);
        QCOMPARE(m.headerFieldText(name), value4);
        QCOMPARE(m.headerFieldsText(name), QStringList(value4));
        QCOMPARE(m.contentModified(), true);
    }
}

static void testAddressHeader(const QString& name, void (QMailMessage::*setter)(const QMailAddress&), QMailAddress (QMailMessage::*getter)() const)
{
    QMailAddress addr1("bob@example.com");
    QMailAddress addr2("Jim <jim@example.com>");

    QMailMessage m;
    QCOMPARE((m.*getter)(), QMailAddress());
    QCOMPARE(m.headerFieldText(name), QString());
    QCOMPARE(m.headerFieldsText(name), QStringList());
    QCOMPARE(m.contentModified(), false);

    (m.*setter)(addr1);
    QCOMPARE((m.*getter)(), addr1);
    QCOMPARE(m.headerFieldText(name), addr1.toString());
    QCOMPARE(m.headerFieldsText(name), QStringList(addr1.toString()));
    QCOMPARE(m.contentModified(), true);

    (m.*setter)(addr2);
    QCOMPARE((m.*getter)(), addr2);
    QCOMPARE(m.headerFieldText(name), addr2.toString());
    QCOMPARE(m.headerFieldsText(name), QStringList(addr2.toString()));
    QCOMPARE(m.contentModified(), true);
}


tst_QMailMessage::tst_QMailMessage()
{
}

tst_QMailMessage::~tst_QMailMessage()
{
}

void tst_QMailMessage::initTestCase()
{
}

void tst_QMailMessage::cleanupTestCase()
{
}

void tst_QMailMessage::init()
{
}

/*?
    Cleanup after each test case.
*/
void tst_QMailMessage::cleanup()
{
}

typedef QPair<QByteArray, QByteArray> pair_type;
Q_DECLARE_METATYPE(pair_type)
Q_DECLARE_METATYPE(QList<pair_type>)

void tst_QMailMessage::toRfc2822_data()
{
    QTest::addColumn<QString>( "from" );
    QTest::addColumn<QStringList>( "to" );
    QTest::addColumn<QString>( "subject" );
    QTest::addColumn<QString>( "time_stamp" );
    QTest::addColumn<QByteArray>( "content_type" );
    QTest::addColumn<QString>( "plain_text" );
    QTest::addColumn<QList<pair_type> >( "text_parts" );
    QTest::addColumn<QByteArray>( "rfc_header_text" );
    QTest::addColumn<QByteArray>( "rfc_body_text" );

    QString simpleAddress("mary@example.net");
    QString latin1Address(QString::fromLatin1("\"Joh\361 D\366e\" <jdoe@example.net>"));

    // Test with some arabic characters, as per http://en.wikipedia.org/wiki/List_of_Unicode_characters
    const QChar chars[] = { 0x0636, 0x0669, 0x06a5, 0x06b4 };
    QString unicodeAddress(chars, 4);
    unicodeAddress.append(" <address@test>");

    QStringList toAddressList;
    toAddressList << unicodeAddress << simpleAddress;

    QTest::newRow("simple") 
        /* from              */ << latin1Address
        /* to                */ << toAddressList
        /* subject           */ << "Test"
        /* time_stamp        */ << "Fri, 21 Nov 1997 09:55:06 -0600"
        /* content_type      */ << QByteArray("text/plain; charset=utf-8")
        /* plain_text        */ << "Plain text."
        /* text_parts        */ << QList<pair_type>()
        /* rfc_header_text   */ << QByteArray(
"From: =?ISO-8859-1?Q?=22Joh=F1_D=F6e=22?= <jdoe@example.net>" CRLF
"To: =?UTF-8?B?2LbZqdql2rQ=?= <address@test>, mary@example.net" CRLF
"Subject: Test" CRLF
"Date: Fri, 21 Nov 1997 09:55:06 -0600" CRLF
"Content-Type: text/plain; charset=utf-8" CRLF
"<ENCODING>"
"MIME-Version: 1.0" CRLF )
        /* rfc_body_text     */ << QByteArray(
"Plain text.");

    QTest::newRow("multipart") 
        /* from              */ << latin1Address
        /* to                */ << toAddressList
        /* subject           */ << "Test"
        /* time_stamp        */ << "Fri, 21 Nov 1997 09:55:06 -0600"
        /* content_type      */ << QByteArray("multipart/alternative; boundary=bound01")
        /* plain_text        */ << ""
        /* text_parts        */ << ( QList<pair_type>() << qMakePair(QByteArray("text/plain"), QByteArray("Hello."))
                                                        << qMakePair(QByteArray("text/html"), QByteArray("<p>Hello.</p>")) )
        /* rfc_header_text   */ << QByteArray(
"From: =?ISO-8859-1?Q?=22Joh=F1_D=F6e=22?= <jdoe@example.net>" CRLF
"To: =?UTF-8?B?2LbZqdql2rQ=?= <address@test>, mary@example.net" CRLF
"Subject: Test" CRLF
"Date: Fri, 21 Nov 1997 09:55:06 -0600" CRLF
"Content-Type: multipart/alternative; boundary=bound01" CRLF
"MIME-Version: 1.0" CRLF
CRLF
"This is a multipart message in Mime 1.0 format" CRLF )
        /* rfc_body_text     */ << QByteArray(
"--bound01" CRLF
"Content-Type: text/plain" CRLF
"<ENCODING>"
"Content-Disposition: inline" CRLF 
CRLF
"<ENCODED_TEXT_0>" CRLF
"--bound01" CRLF
"Content-Type: text/html" CRLF
"<ENCODING>"
"Content-Disposition: inline" CRLF 
CRLF
"<ENCODED_TEXT_1>" CRLF
"--bound01--" CRLF);
}

//    virtual QString toRfc2822() const;
void tst_QMailMessage::toRfc2822()
{
    QFETCH(QString, from);
    QFETCH(QStringList, to);
    QFETCH(QString, subject);
    QFETCH(QString, time_stamp);
    QFETCH(QByteArray, content_type);
    QFETCH(QString, plain_text);
    QFETCH(QList<pair_type>, text_parts);
    QFETCH(QByteArray, rfc_header_text);
    QFETCH(QByteArray, rfc_body_text);

    QMailMessageBody::TransferEncoding te[3] = { QMailMessageBody::NoEncoding, QMailMessageBody::Base64, QMailMessageBody::QuotedPrintable };
    for (int i = 0; i < 2; ++i)
    {
        QMailMessage message;
        message.setFrom(QMailAddress(from));
        message.setTo(QMailAddress::fromStringList(to));
        message.setSubject(subject);
        message.setDate(QMailTimeStamp(time_stamp));
        QMailMessageContentType type(content_type);

        if (type.type().toLower() == "multipart") {
            message.setBody(QMailMessageBody::fromData(QString(), type, QMailMessageBody::NoEncoding));

            QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Inline);

            foreach (const pair_type &pair, text_parts) {
                QMailMessageContentType type(pair.first);
                QMailMessagePart part = QMailMessagePart::fromData(pair.second, disposition, type, te[i]);
                message.appendPart(part);
            }
        } else {
            message.setBody(QMailMessageBody::fromData(plain_text, type, te[i]));
        }

        QByteArray rfc_text(rfc_header_text);
        rfc_text.append(CRLF);

        QByteArray body_text(rfc_body_text);
        {
            if (te[i] == QMailMessageBody::Base64) {
                rfc_text.replace("<ENCODING>", "Content-Transfer-Encoding: base64" CRLF);
                body_text.replace("<ENCODING>", "Content-Transfer-Encoding: base64" CRLF);

                if (message.multipartType() == QMailMessage::MultipartNone) {
                    QMailBase64Codec codec(QMailBase64Codec::Text);
                    rfc_text.append(codec.encode(body_text, "UTF-8"));
                } else {
                    int i = 0;
                    foreach (const pair_type &pair, text_parts) {
                        QString tag("<ENCODED_TEXT_%1>");
                        QMailBase64Codec codec(QMailBase64Codec::Text);
                        body_text.replace(tag.arg(i).toLatin1(), codec.encode(pair.second, "UTF-8"));
                        ++i;
                    }
                    rfc_text.append(body_text);
                }
            } else if (te[i] == QMailMessageBody::QuotedPrintable ) {
                rfc_text.replace("<ENCODING>", "Content-Transfer-Encoding: quoted-printable" CRLF);
                body_text.replace("<ENCODING>", "Content-Transfer-Encoding: quoted-printable" CRLF);

                if (message.multipartType() == QMailMessage::MultipartNone) {
                    QMailQuotedPrintableCodec codec(QMailQuotedPrintableCodec::Text, QMailQuotedPrintableCodec::Rfc2045);
                    rfc_text.append(codec.encode(body_text, "UTF-8"));
                } else {
                    int i = 0;
                    foreach (const pair_type &pair, text_parts) {
                        QString tag("<ENCODED_TEXT_%1>");
                        QMailQuotedPrintableCodec codec(QMailQuotedPrintableCodec::Text, QMailQuotedPrintableCodec::Rfc2045);
                        body_text.replace(tag.arg(i).toLatin1(), codec.encode(pair.second, "UTF-8"));
                        ++i;
                    }
                    rfc_text.append(body_text);
                }
            } else {
                rfc_text.replace("<ENCODING>", "");
                body_text.replace("<ENCODING>", "");

                if (message.multipartType() == QMailMessage::MultipartNone) {
                    rfc_text.append(body_text);
                } else {
                    int i = 0;
                    foreach (const pair_type &pair, text_parts) {
                        QString tag("<ENCODED_TEXT_%1>");
                        body_text.replace(tag.arg(i).toLatin1(), pair.second);
                        ++i;
                    }
                    rfc_text.append(body_text);
                }
            }
        }

        QByteArray encoded = message.toRfc2822();
        QCOMPARE(encoded, rfc_text);

        // Test that conversion to-and-from RFC2822 yields equivalence
        QByteArray identity = message.toRfc2822(QMailMessage::IdentityFormat);

        QMailMessage reconstituted = QMailMessage::fromRfc2822(identity);
        QCOMPARE( reconstituted.from(), message.from() );
        QCOMPARE( QMailAddress::toStringList(reconstituted.to()), QMailAddress::toStringList(message.to()) );
        QCOMPARE( QMailAddress::toStringList(reconstituted.cc()), QMailAddress::toStringList(message.cc()) );
        QCOMPARE( QMailAddress::toStringList(reconstituted.bcc()), QMailAddress::toStringList(message.bcc()) );
        QCOMPARE( reconstituted.subject(), message.subject() );
        QCOMPARE( reconstituted.replyTo().toString(), message.replyTo().toString() );
        QCOMPARE( reconstituted.inReplyTo(), message.inReplyTo() );
        QCOMPARE( reconstituted.headerFieldText("Message-ID"), message.headerFieldText("Message-ID") );
        QCOMPARE( QMailTimeStamp(reconstituted.date()), QMailTimeStamp(message.date()) );
        QCOMPARE( reconstituted.hasBody(), message.hasBody() );
        QCOMPARE( reconstituted.body().data(), message.body().data() );
        QCOMPARE( reconstituted.partCount(), message.partCount() );
    }
}

void tst_QMailMessage::fromRfc2822_data()
{
    QTest::addColumn<QByteArray>( "rfc_text" );
    QTest::addColumn<QString>( "from" );
    QTest::addColumn<QString>( "from_name" );
    QTest::addColumn<QString>( "from_email" );
    QTest::addColumn<QStringList>( "to" );
    QTest::addColumn<QStringList>( "cc" );
    QTest::addColumn<QStringList>( "bcc" );
    QTest::addColumn<QString>( "subject" );
    QTest::addColumn<QString>( "reply_to" );
    QTest::addColumn<QString>( "in_reply_to" );
    QTest::addColumn<QString>( "message_id" );
    QTest::addColumn<QDateTime>( "datetime" );
    QTest::addColumn<QString>( "date_str" );
    QTest::addColumn<int>( "type" );
    QTest::addColumn<QString>( "plain_text" );
    QTest::addColumn<int>( "message_part_count" );

    QTest::newRow("sample_1") << QByteArray(
"From: John Doe <jdoe@machine.example>" CRLF
"To: Mary Smith <mary@example.net>" CRLF
"Subject: Saying Hello" CRLF
"Date: Fri, 21 Nov 1997 09:55:06 -0600" CRLF
"Message-ID: <1234@local.machine.example>" CRLF
CRLF
"This is a message just to say hello.\015"
"So, ""Hello"".") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<1234@local.machine.example>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(15,55,6), Qt::UTC).toLocalTime() // datetime
                      << QString( "Fri, 21 Nov 1997 09:55:06 -0600" ) // date_str
                      << int(QMailMessage::Email) // type
                      << QString("This is a message just to say hello.\nSo, ""Hello"".") // plain_text
                      << int( 0 ); // message_part_count

/*
    This sample adds a Sender field. We don't seem to do anything with it.
*/
    QTest::newRow("sample_2") << QByteArray(
"From: John Doe <jdoe@machine.example>" CRLF
"Sender: Michael Jackson <mjackson@machine.example>" CRLF
"To: Mary Smith <mary@example.net>" CRLF
"Subject: Saying Hello" CRLF
"Date: Sat, 22 Nov 1997 09:55:16 -0600" CRLF
"Message-ID: <1234@local.machine.example>" CRLF
CRLF
"This is a message just to say hello." CRLF
"So, ""Hello"".") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<1234@local.machine.example>") // message_id
                      << QDateTime(QDate(1997,11,22), QTime(15,55,16), Qt::UTC).toLocalTime() // datetime
                      << QString( "Sat, 22 Nov 1997 09:55:16 -0600" ) // date_str
                      << int(QMailMessage::Email) // type
                      << QString("This is a message just to say hello.\nSo, ""Hello"".") // plain_text
                      << int( 0 ); // message_part_count

/*
    This example uses complex to and cc fields.
*/
    QTest::newRow("sample_3") << QByteArray(
"From: \"Joe Q. Public\" <john.q.public@example.com>" CRLF
"To: Mary Smith <mary(comment ( nested-comment  ))@x.test>, jdoe@example.org, Who? <one@y.test>" CRLF
"Cc: <boss@nil.test>, \"Giant; \"Big\" Box\" <sysservices@example.net>" CRLF
"Date: Tue, 1 Jul 2003 10:52:37 +0200" CRLF
"Message-ID: <5678.21-Nov-1997@example.com>" CRLF
CRLF
"Hi everyone.") // rfc_text
                      << QString("\"Joe Q. Public\" <john.q.public@example.com>") // from
                      << QString("Joe Q. Public") // from_name
                      << QString("john.q.public@example.com") // from_email
                      << QStringList( QString("Mary Smith <mary(comment ( nested-comment  ))@x.test>,jdoe@example.org,Who? <one@y.test>").split(",") ) // to
                      << QStringList( QString("boss@nil.test,\"Giant; \"Big\" Box\" <sysservices@example.net>").split(",") ) // cc
                      << QStringList() // bcc
                      << QString() // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<5678.21-Nov-1997@example.com>") // message_id
                      << QDateTime(QDate(2003,7,1), QTime(8,52,37), Qt::UTC).toLocalTime() // datetime
                      << QString("Tue, 1 Jul 2003 10:52:37 +0200") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Hi everyone.") // plain_text
                      << int( 0 ); // message_part_count

/*
    This sample uses group names.
*/
    QTest::newRow("sample_4") << QByteArray(
"From: Pete <pete@silly.example>" CRLF
"To: A Group:Chris Jones <c@a.test>,joe@where.test,John <jdoe@one.test>;" CRLF
"Cc: \"Undisclosed recipients\":;" CRLF
"Date: Thu, 13 Feb 1969 23:32:54 -0330" CRLF
"Message-ID: <testabcd.1234@silly.example>" CRLF
CRLF
"Testing.") // rfc_text
                      << QString("Pete <pete@silly.example>") // from
                      << QString("Pete") // from_name
                      << QString("pete@silly.example") // from_email
                      << QStringList("A Group: Chris Jones <c@a.test>,joe@where.test,John <jdoe@one.test>;")
                      << QStringList("\"Undisclosed recipients\": ;") // cc
                      << QStringList() // bcc
                      << QString() // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<testabcd.1234@silly.example>") // message_id
                      << QDateTime(QDate(1969,2,14), QTime(3,2,54), Qt::UTC).toLocalTime() // datetime
                      << QString("Thu, 13 Feb 1969 23:32:54 -0330") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Testing.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("reply_test_1") << QByteArray(
"From: John Doe <jdoe@machine.example>" CRLF
"To: Mary Smith <mary@example.net>" CRLF
"Subject: Saying Hello" CRLF
"Date: Fri, 21 Nov 1997 09:55:06 -0600" CRLF
"Message-ID: <1234@local.machine.example>" CRLF
CRLF
"Hello.") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<1234@local.machine.example>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(15,55,6), Qt::UTC).toLocalTime() // datetime
                      << QString("Fri, 21 Nov 1997 09:55:06 -0600") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Hello.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("reply_test_2") << QByteArray(
"From: Mary Smith <mary@example.net>" CRLF
"To: John Doe <jdoe@machine.example>" CRLF
"Reply-To: \"Mary Smith: Personal Account\" <smith@home.example>" CRLF
"Subject: Re: Saying Hello" CRLF
"Date: Fri, 21 Nov 1997 10:01:10 -0600" CRLF
"Message-ID: <3456@example.net>" CRLF
"In-Reply-To: <1234@local.machine.example>" CRLF
"References: <1234@local.machine.example>" CRLF
CRLF
"This is a reply to your hello.") // rfc_text
                      << QString("Mary Smith <mary@example.net>") // from
                      << QString("Mary Smith") // from_name
                      << QString("mary@example.net") // from_email
                      << QStringList("John Doe <jdoe@machine.example>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Re: Saying Hello") // subject
                      << QString("\"Mary Smith: Personal Account\" <smith@home.example>") // reply_to
                      << QString("<1234@local.machine.example>") // in_reply_to
                      << QString("<3456@example.net>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(16,1,10), Qt::UTC).toLocalTime() // datetime
                      << QString("Fri, 21 Nov 1997 10:01:10 -0600") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("This is a reply to your hello.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("reply_test_3") << QByteArray(
"To: \"Mary Smith: Personal Account\" <smith@home.example>" CRLF
"From: John Doe <jdoe@machine.example>" CRLF
"Subject: Re: Saying Hello" CRLF
"Date: Fri, 21 Nov 1997 11:00:00 -0600" CRLF
"Message-ID: <abcd.1234@local.machine.tld>" CRLF
"In-Reply-To: <3456@example.net>" CRLF
"References: <1234@local.machine.example> <3456@example.net>" CRLF
CRLF
"This is a reply to your reply.") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("\"Mary Smith: Personal Account\" <smith@home.example>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Re: Saying Hello") // subject
                      << QString() // reply_to
                      << QString("<3456@example.net>") // in_reply_to
                      << QString("<abcd.1234@local.machine.tld>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(17,0,0), Qt::UTC).toLocalTime() // datetime
                      << QString("Fri, 21 Nov 1997 11:00:00 -0600") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("This is a reply to your reply.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("resent_test_1") << QByteArray(
"Resent-From: Mary Smith <mary@example.net>" CRLF
"Resent-To: Jane Brown <j-brown@other.example>" CRLF
"Resent-Date: Mon, 24 Nov 1997 14:22:01 -0800" CRLF
"Resent-Message-ID: <78910@example.net>" CRLF
"From: John Doe <jdoe@machine.example>" CRLF
"To: Mary Smith <mary@example.net>" CRLF
"Subject: Saying Hello" CRLF
"Date: Fri, 21 Nov 1997 09:55:06 -0600" CRLF
"Message-ID: <1234@local.machine.example>" CRLF
CRLF
"Hello.") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<1234@local.machine.example>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(15,55,6), Qt::UTC).toLocalTime() // datetime
                      << QString("Fri, 21 Nov 1997 09:55:06 -0600") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Hello.") // plain_text
                      << int(0); // message_part_count

    QTest::newRow("whitespace_test_1") << QByteArray(
"From: Pete(A \\\\wonderful \\) chap) <pete(his account)@silly.test(his host)>" CRLF
"To:A Group(Some people)" CRLF
"     :Chris Jones <c@(Chris's host.)public.example>," CRLF
"         joe@example.org," CRLF
"  John <jdoe@one.test> (my dear friend); (the end of the group)" CRLF
"Cc:(Empty list)(start)\"Undisclosed recipients  \":(nobody (that I know))  ;" CRLF
"Date: Thu," CRLF
"      13" CRLF
"        Feb" CRLF
"          1969" CRLF
"      23:32" CRLF
"               -0330 (Newfoundland Time)" CRLF
"Message-ID:              <testabcd.1234@silly.test>" CRLF
CRLF
"Testing.") // rfc_text
                      << QString("Pete(A \\\\wonderful \\) chap) <pete(his account)@silly.test(his host)>") // from
                      << QString("Pete(A \\\\wonderful \\) chap)") // from_name
                      << QString("pete(his account)@silly.test(his host)") // from_email
                      << QStringList(QString(
"A Group(Some people): Chris Jones <c@(Chris's host.)public.example>,"
"         joe@example.org,"
"  John <jdoe@one.test> (my dear friend);"))
                      << QStringList(QString("(Empty list)(start)\"Undisclosed recipients  \": (nobody (that I know));")) // cc
                      << QStringList() // bcc
                      << QString() // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<testabcd.1234@silly.test>") // message_id
                      << QDateTime(QDate(1969,2,14), QTime(3,2), Qt::UTC).toLocalTime() // datetime
                      << QString("Thu, 13 Feb 1969 23:32:00 -0330") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Testing.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("obsolete_address_1") << QByteArray(
"From: Joe Q. Public <john.q.public@example.com>" CRLF
"To: Mary Smith <@machine.tld:mary@example.net>, , jdoe@test   . example" CRLF
"Date: Tue, 1 Jul 2003 10:52:37 +0200" CRLF
"Message-ID: <5678.21-Nov-1997@example.com>" CRLF
CRLF
"Hi everyone.") // rfc_text
                      << QString("\"Joe Q. Public\" <john.q.public@example.com>") // from
                      << QString("Joe Q. Public") // from_name
                      << QString("john.q.public@example.com") // from_email
                      << QStringList(QString("Mary Smith <@machine.tld:mary@example.net>,jdoe@test   . example").split(",")) // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString() // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<5678.21-Nov-1997@example.com>") // message_id
                      << QDateTime(QDate(2003,7,1), QTime(8,52,37), Qt::UTC).toLocalTime() // datetime
                      << QString("Tue, 1 Jul 2003 10:52:37 +0200") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Hi everyone.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("obsolete_date_1") << QByteArray(
"From: John Doe <jdoe@machine.example>" CRLF
"To: Mary Smith <mary@example.net>" CRLF
"Subject: Saying Hello" CRLF
"Date: 21 Nov 97 09:55:06 GMT" CRLF
"Message-ID: <1234@local.machine.example>" CRLF
CRLF
"Hello.") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<1234@local.machine.example>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(9,55,6), Qt::UTC).toLocalTime() // datetime
                      << QString("Fri, 21 Nov 1997 09:55:06 +0000") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Hello.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("obsolete_whitespace_1") << QByteArray(
"From  : John Doe <jdoe@machine(comment).  example>" CRLF
"To    : Mary Smith" CRLF
"  " CRLF
"          <mary@example.net>" CRLF
"Subject     : Saying Hello" CRLF
"Date  : Fri, 21 Nov 1997 09(comment):55:06 -0600" CRLF // Note - spaces between hh:mm:ss are illegal
"Message-ID  : <1234   @   local(blah)  .machine .example>" CRLF
CRLF
"Obsolete whitespaces everywhere.") // rfc_text
                      << QString("John Doe <jdoe@machine(comment).  example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine(comment).  example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<1234   @   local(blah)  .machine .example>") // message_id (unprocessed)
                      << QDateTime(QDate(1997,11,21), QTime(15,55,6), Qt::UTC).toLocalTime() // datetime
                      << QString("Fri, 21 Nov 1997 09:55:06 -0600") // date_str
                      << int(QMailMessage::Email) // type
                      << QString("Obsolete whitespaces everywhere.") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("encoded_words") << QByteArray(
"From:=?UTF-8?Q?J=6fhn=20D=6fe?= <jdoe@machine.example>" CRLF
"To:=?UTF-8?Q?Mary_S=6dith?= <mary@example.net>" CRLF
"Subject:=?\"UTF-8\"?B?U2F5aW5nIEhlbGxv?= =?ISO-8859-1?Q?_Encoded_?= =?UTF-8?Q?=41=42=43?=" CRLF
"Date: Fri, 21 Nov 1997 09:55:06 -0600" CRLF
"Message-ID: <1234@local.machine.example>" CRLF
CRLF
"This is a message just to say hello." CRLF
"So, ""Hello"".") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello Encoded ABC") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<1234@local.machine.example>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(15,55,6), Qt::UTC).toLocalTime() // datetime
                      << QString( "Fri, 21 Nov 1997 09:55:06 -0600" ) // date_str
                      << int(QMailMessage::Email) // type
                      << QString("This is a message just to say hello.\nSo, ""Hello"".") // plain_text
                      << int( 0 ); // message_part_count

    QTest::newRow("long_header") << QByteArray(
"From: John Doe <jdoe@machine.example>" CRLF
"To: Mary Smith <mary@example.net>" CRLF
"Subject: Saying Hello" CRLF
"Date: Fri, 21 Nov 1997 09:55:06 -0600" CRLF
"Message-ID: " CRLF
" <D5D122C46A7E494280C389109EA6190705C1C4C72C01ABCDEF@ex2007.ex2007.pohjola.example.com>" CRLF
CRLF
"This is a message just to say hello.\015"
"So, ""Hello"".") // rfc_text
                      << QString("John Doe <jdoe@machine.example>") // from
                      << QString("John Doe") // from_name
                      << QString("jdoe@machine.example") // from_email
                      << QStringList("Mary Smith <mary@example.net>") // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString("Saying Hello") // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString("<D5D122C46A7E494280C389109EA6190705C1C4C72C01ABCDEF@ex2007.ex2007.pohjola.example.com>") // message_id
                      << QDateTime(QDate(1997,11,21), QTime(15,55,6), Qt::UTC).toLocalTime() // datetime
                      << QString( "Fri, 21 Nov 1997 09:55:06 -0600" ) // date_str
                      << int(QMailMessage::Email) // type
                      << QString("This is a message just to say hello.\nSo, ""Hello"".") // plain_text
                      << int( 0 ); // message_part_count

/* TEMPLATE TO ADD A NEW ROW
    QTest::newRow("") << QByteArray("") // rfc_text
                      << QString() // from
                      << QString() // from_name
                      << QString() // from_email
                      << QStringList() // to
                      << QStringList() // cc
                      << QStringList() // bcc
                      << QString() // subject
                      << QString() // reply_to
                      << QString() // in_reply_to
                      << QString() // message_id
                      << QDateTime(QDate(),QTime()) // datetime
                      << QString() // date_str
                      << int() // type
                      << QString() // plain_text
                      << int(); // message_part_count
*/
}

//    virtual void fromRfc2822(const QByteArray &ba);
void tst_QMailMessage::fromRfc2822()
{
    QFETCH( QByteArray, rfc_text );
    QMailMessage m = QMailMessage::fromRfc2822( rfc_text );

    QTEST( m.from().toString(), "from" );
    QTEST( m.from().name(), "from_name" );
    QTEST( m.from().address(), "from_email" );
    QTEST( QMailAddress::toStringList(m.to()), "to" );
    QTEST( QMailAddress::toStringList(m.cc()), "cc" );
    QTEST( QMailAddress::toStringList(m.bcc()), "bcc" );
    QTEST( m.subject(), "subject" );
    QTEST( m.replyTo().toString(), "reply_to" );
    QTEST( m.inReplyTo(), "in_reply_to" );
    QTEST( m.headerFieldText("Message-ID"), "message_id" );
    QTEST( QMailTimeStamp(m.date()).toLocalTime(), "datetime" );
    QTEST( QMailTimeStamp(m.date()).toString(), "date_str" );
    QTEST( m.body().data(), "plain_text" );
    QTEST( int(m.partCount()), "message_part_count" );

    QFETCH(QStringList, to);
    QFETCH(QStringList, cc);
    QFETCH(QStringList, bcc);
    QMailMessageMetaData metaData = *static_cast<QMailMessageMetaData*>(&m);
    QCOMPARE(QMailAddress::toStringList(metaData.recipients()), (to + cc + bcc));


    // Test that conversion to-and-from RFC2822 yields equivalence
    QByteArray identity = m.toRfc2822(QMailMessage::IdentityFormat);

    QMailMessage reconstituted = QMailMessage::fromRfc2822(identity);
    QCOMPARE( reconstituted.from(), m.from() );
    QCOMPARE( QMailAddress::toStringList(reconstituted.to()), QMailAddress::toStringList(m.to()) );
    QCOMPARE( QMailAddress::toStringList(reconstituted.cc()), QMailAddress::toStringList(m.cc()) );
    QCOMPARE( QMailAddress::toStringList(reconstituted.bcc()), QMailAddress::toStringList(m.bcc()) );
    QCOMPARE( reconstituted.subject(), m.subject() );
    QCOMPARE( reconstituted.replyTo().toString(), m.replyTo().toString() );
    QCOMPARE( reconstituted.inReplyTo(), m.inReplyTo() );
    QCOMPARE( reconstituted.headerFieldText("Message-ID"), m.headerFieldText("Message-ID") );
    QCOMPARE( QMailTimeStamp(reconstituted.date()), QMailTimeStamp(m.date()) );
    QCOMPARE( reconstituted.hasBody(), m.hasBody() );
    QCOMPARE( reconstituted.body().data(), m.body().data() );
    QCOMPARE( reconstituted.partCount(), m.partCount() );
}

void tst_QMailMessage::id()
{
    // Tested by: setId
}

// The real QMailStore (which we don't need here) has friend access to the QMailMessageId ctor
class QMailStore 
{
public:
    static QMailMessageId getId(quint64 value) { return QMailMessageId(value); }
    static QMailFolderId getFolderId(quint64 value) { return QMailFolderId(value); }
};

void tst_QMailMessage::setId()
{
    QMailMessageId id;
    QMailMessage message;
    QCOMPARE( message.id(), id );
    QCOMPARE( message.dataModified(), false );

    id = QMailStore::getId(100);
    message.setId(id);
    QCOMPARE( message.id(), id );
    QCOMPARE( message.dataModified(), true );
}

void tst_QMailMessage::parentFolderId()
{
    // Tested by: setParentFolderId
}

void tst_QMailMessage::setParentFolderId()
{
    QMailFolderId id;
    QMailMessage message;
    QCOMPARE( message.parentFolderId(), id );
    QCOMPARE( message.dataModified(), false );

    id = QMailStore::getFolderId(200);
    message.setParentFolderId(id);
    QCOMPARE( message.parentFolderId(), id );
    QCOMPARE( message.dataModified(), true );
}

void tst_QMailMessage::messageType()
{
    // Tested by: setMessageType
}

void tst_QMailMessage::setMessageType_data()
{
    QTest::addColumn<QMailMessage::MessageType>( "type" );

    QTest::newRow("SMS") << QMailMessage::Sms;
    QTest::newRow("MMS") << QMailMessage::Mms;
    QTest::newRow("Email") << QMailMessage::Email;
    QTest::newRow("System") << QMailMessage::System;
}

void tst_QMailMessage::setMessageType()
{
    QFETCH(QMailMessage::MessageType, type);

    QMailMessage message;
    QCOMPARE( message.messageType(), QMailMessage::None );
    QCOMPARE( message.dataModified(), false );

    message.setMessageType(type);
    QCOMPARE( message.messageType(), type );
    QCOMPARE( message.dataModified(), true );
}

void tst_QMailMessage::from()
{
    // Tested by: setFrom
}

void tst_QMailMessage::setFrom()
{
    testAddressHeader("From", &QMailMessage::setFrom, &QMailMessage::from);
}

void tst_QMailMessage::subject()
{
    // Tested by: setSubject
}

void tst_QMailMessage::setSubject()
{
    testHeader("Subject", &QMailMessage::setSubject, &QMailMessage::subject);
}

void tst_QMailMessage::date()
{
    // Tested by: setDate
}

void tst_QMailMessage::setDate()
{
    QString value1("Thu, 13 Feb 1969 23:32:00 -0330");
    QString value2("Fri, 21 Nov 1997 09:55:06 +0000");

    QMailMessage m;
    QCOMPARE(m.date().toString(), QString());
    QCOMPARE(m.headerFieldText("Date"), QString());
    QCOMPARE(m.headerFieldsText("Date"), QStringList());
    QCOMPARE(m.dataModified(), false);
    QCOMPARE(m.contentModified(), false);

    m.setDate(QMailTimeStamp(value1));
    QCOMPARE(m.date().toString(), value1);
    QCOMPARE(m.headerFieldText("Date"), value1);
    QCOMPARE(m.headerFieldsText("Date"), QStringList(value1));
    QCOMPARE(m.dataModified(), true);
    QCOMPARE(m.contentModified(), true);

    m.setDate(QMailTimeStamp(value2));
    QCOMPARE(m.date().toString(), value2);
    QCOMPARE(m.headerFieldText("Date"), value2);
    QCOMPARE(m.headerFieldsText("Date"), QStringList(value2));
    QCOMPARE(m.dataModified(), true);
    QCOMPARE(m.contentModified(), true);
}

void tst_QMailMessage::to()
{
    // Tested by: setTo
}

void tst_QMailMessage::setTo_data()
{
    QTest::addColumn<QList<QMailAddress> >( "list" );

    QTest::newRow("empty")
        << QList<QMailAddress>();

    QTest::newRow("simple")
        << ( QList<QMailAddress>() << QMailAddress("hello") );

    QTest::newRow("multiple")
        << ( QList<QMailAddress>() << QMailAddress("hello") << QMailAddress("world") );

    QTest::newRow("morethan78charactersofconsecutivenonwhitespace")
        << ( QList<QMailAddress>() << QMailAddress("\"dummy inbox444\" <dummy.inbox444@gmail.com")
             << QMailAddress("ext-varaprasad.2.killampalli@gmail.com")
             << QMailAddress("sadhukills@yahoo.com")
             << QMailAddress("groovetest007@gmail.com")
             << QMailAddress("dummy.dummy5555@gmail.com")
             << QMailAddress("don.sanders@nokia.com") );

    QTest::newRow("realistic")
        << ( QList<QMailAddress>() << QMailAddress("John Doe <jdoe@machine.example>")
                                   << QMailAddress("Mary Smith <mary@example.net>")
                                   << QMailAddress("Mary Smith <mary(comment ( nested-comment  ))@x.test>")
                                   << QMailAddress("\"Mary Smith: Personal Account\" <smith@home.example>")
                                   << QMailAddress("Who? <one@y.test>") );

    QTest::newRow("group")
        << ( QList<QMailAddress>() << QMailAddress("John Doe <jdoe@machine.example>")
                                   << QMailAddress("A Group: Chris Jones <c@a.test>,joe@where.test,John <jdoe@one.test>;")
                                   << QMailAddress("Mary Smith <mary@example.net>") );
}

void tst_QMailMessage::setTo()
{
    QFETCH( QList<QMailAddress>, list );

    {
        QMailMessage m;
        QCOMPARE(QMailAddress::toStringList(m.to()).join(","), QString());
        QCOMPARE(m.headerFieldText("To"), QString());
        QCOMPARE(m.dataModified(), false);
        QCOMPARE(m.contentModified(), false);

        m.setTo(list);
        QCOMPARE(m.to(), list);
        QCOMPARE(QMailAddress::fromStringList(m.headerFieldText("To")), list);
        if (!list.isEmpty()) {
            QCOMPARE(m.dataModified(), true);
            QCOMPARE(m.contentModified(), true);
        }
        
        // Test that conversion to-and-from RFC2822 yields equivalence
        QByteArray identity = m.toRfc2822(QMailMessage::IdentityFormat);
        QMailMessage reconstituted = QMailMessage::fromRfc2822(identity);
        QCOMPARE(QMailAddress::fromStringList(reconstituted.headerFieldText("To")), list);
    }

    {
        QMailMessage m;
        QCOMPARE(QMailAddress::toStringList(m.cc()).join(","), QString());
        QCOMPARE(m.headerFieldText("CC"), QString());
        QCOMPARE(m.contentModified(), false);

        m.setCc(list);
        QCOMPARE(m.cc(), list);
        QCOMPARE(QMailAddress::fromStringList(m.headerFieldText("CC")), list);
        if (!list.isEmpty()) {
            QCOMPARE(m.contentModified(), true);
        }
    }

    {
        QMailMessage m;
        QCOMPARE(QMailAddress::toStringList(m.bcc()).join(","), QString());
        QCOMPARE(m.headerFieldText("BCC"), QString());
        QCOMPARE(m.contentModified(), false);

        m.setBcc(list);
        QCOMPARE(m.bcc(), list);
        QCOMPARE(QMailAddress::fromStringList(m.headerFieldText("BCC")), list);
        if (!list.isEmpty()) {
            QCOMPARE(m.contentModified(), true);
        }
    }
}

void tst_QMailMessage::cc()
{
    // Tested by: setTo
}

void tst_QMailMessage::setCc()
{
    // Tested by: setTo
}

void tst_QMailMessage::bcc()
{
    // Tested by: setTo
}

void tst_QMailMessage::setBcc()
{
    // Tested by: setTo
}

void tst_QMailMessage::replyTo()
{
    // Tested by: setReplyTo
}

void tst_QMailMessage::recipients_data()
{
    QTest::addColumn<QList<QMailAddress> >( "to" );
    QTest::addColumn<QList<QMailAddress> >( "cc" );
    QTest::addColumn<QList<QMailAddress> >( "bcc" );

    QTest::newRow("empty")
        << QList<QMailAddress>()
        << QList<QMailAddress>()
        << QList<QMailAddress>();

    QTest::newRow("To only")
        << ( QList<QMailAddress>() << QMailAddress("hello") )
        << QList<QMailAddress>()
        << QList<QMailAddress>();

    QTest::newRow("CC only")
        << QList<QMailAddress>()
        << ( QList<QMailAddress>() << QMailAddress("hello") )
        << QList<QMailAddress>();

    QTest::newRow("BCC only")
        << QList<QMailAddress>()
        << QList<QMailAddress>()
        << ( QList<QMailAddress>() << QMailAddress("hello") );

    QTest::newRow("multiple")
        << QList<QMailAddress>()
        << ( QList<QMailAddress>() << QMailAddress("hello") )
        << ( QList<QMailAddress>() << QMailAddress("hello") );

    QTest::newRow("all")
        << ( QList<QMailAddress>() << QMailAddress("hello") )
        << ( QList<QMailAddress>() << QMailAddress("hello") )
        << ( QList<QMailAddress>() << QMailAddress("hello") );
}

void tst_QMailMessage::recipients()
{
    QFETCH( QList<QMailAddress>, to );
    QFETCH( QList<QMailAddress>, cc );
    QFETCH( QList<QMailAddress>, bcc );

    QMailMessage m;
    QCOMPARE(m.hasRecipients(), false);
    QCOMPARE(m.recipients(), QList<QMailAddress>());

    m.setTo(to);
    QCOMPARE(m.to(),to);
    m.setCc(cc);
    QCOMPARE(m.cc(), cc);
    m.setBcc(bcc);
    QCOMPARE(m.bcc(), bcc);
    QCOMPARE(m.hasRecipients(), (!to.isEmpty() || !cc.isEmpty() || !bcc.isEmpty()));
    QCOMPARE(m.recipients(), (to + cc + bcc));
    QMailMessageMetaData metaData = *static_cast<QMailMessageMetaData*>(&m);
    QCOMPARE(metaData.recipients(), (to + cc + bcc));
}

void tst_QMailMessage::hasRecipients()
{
    // Tested by: recipients
}

void tst_QMailMessage::setReplyTo()
{
    testAddressHeader("Reply-To", &QMailMessage::setReplyTo, &QMailMessage::replyTo);
}

void tst_QMailMessage::inReplyTo()
{
    // Tested by: setInReplyTo
}

void tst_QMailMessage::setInReplyTo()
{
    testHeader("In-Reply-To", &QMailMessage::setInReplyTo, &QMailMessage::inReplyTo, true);
}

void tst_QMailMessage::serverUid()
{
    // Tested by: setServerUid
}

void tst_QMailMessage::setServerUid()
{
    testHeader(QString(), &QMailMessage::setServerUid, &QMailMessage::serverUid);
}

void tst_QMailMessage::multiMultipart()
{
// Define this to produce a message which does not exceed the qDebug buffer limit
//#define FIT_MESSAGE_WITHIN_QDEBUG_LIMIT

    QByteArray data;
    QByteArray type;

    QMailMessagePart p1;
    type = "text/plain; charset=UTF-8";
    data = "P1: This is a plain text part.", 
    p1.setBody(QMailMessageBody::fromData(data, QMailMessageContentType(type), QMailMessageBody::EightBit, QMailMessageBody::RequiresEncoding));
    QCOMPARE( p1.contentType().toString().toLower(), QByteArray("Content-Type: text/plain; charset=UTF-8").toLower() );
    QCOMPARE( p1.transferEncoding(), QMailMessageBody::EightBit );
    QCOMPARE( p1.partCount(), static_cast<uint>(0) );
    for (uint i = 0; i < p1.partCount(); ++i)
        QCOMPARE( p1.partAt(i).partNumber(), static_cast<int>(i) );

    QMailMessagePart p2;

    QMailMessagePart p3;
    type = "text/html; charset=UTF-8";
    data = 
#ifndef FIT_MESSAGE_WITHIN_QDEBUG_LIMIT
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
"    <head>"
"        <title>HTML Text Part</title>"
"    </head>"
"    <body>"
"P3: This part should refer to p4 to make sense...\n"
"    </body>"
#else
"<html>P3: This part should refer to p4 to make sense..."
#endif
"</html>";
    p3.setBody(QMailMessageBody::fromData(data, QMailMessageContentType(type), QMailMessageBody::EightBit, QMailMessageBody::RequiresEncoding));
    QCOMPARE( p3.contentType().toString().toLower(), QByteArray("Content-Type: text/html; charset=UTF-8").toLower() );
    QCOMPARE( p3.transferEncoding(), QMailMessageBody::EightBit );

    QMailMessagePart p4;
    type = "text/plain;\n charset=\"ISO-8859-1\"";
    data = "P4: This is a plain text part that should be referenced by p3...";
    p4.setBody(QMailMessageBody::fromData(data, QMailMessageContentType(type), QMailMessageBody::EightBit, QMailMessageBody::RequiresEncoding));
    QCOMPARE( p4.contentType().toString().toLower(), QByteArray("Content-Type: text/plain; charset=ISO-8859-1").toLower() );
    QCOMPARE( p4.transferEncoding(), QMailMessageBody::EightBit );

    QMailMessagePart p5;
    type = "text/plain;\n charset=\"us-ascii\"";
    p5.setBody(QMailMessageBody::fromFile("/etc/hosts", QMailMessageContentType(type), QMailMessageBody::SevenBit, QMailMessageBody::RequiresEncoding));
    QCOMPARE( p5.contentType().toString().toLower(), QByteArray("Content-Type: text/plain; charset=us-ascii").toLower() );
    QCOMPARE( p5.transferEncoding(), QMailMessageBody::SevenBit );

    p2.setMultipartType(QMailMessagePartContainer::MultipartRelated);
    p2.appendPart(p3);
    p2.appendPart(p4);
#ifndef FIT_MESSAGE_WITHIN_QDEBUG_LIMIT
    p2.appendPart(p5);
#endif
    QCOMPARE( p2.contentType().toString().toLower(), QByteArray("Content-Type: multipart/related").toLower() );
    QCOMPARE( p2.transferEncoding(), QMailMessageBody::NoEncoding );
#ifndef FIT_MESSAGE_WITHIN_QDEBUG_LIMIT
    QCOMPARE( p2.partCount(), static_cast<uint>(3) );
#else
    QCOMPARE( p2.partCount(), static_cast<uint>(2) );
#endif
    for (uint i = 0; i < p2.partCount(); ++i)
        QCOMPARE( p2.partAt(i).partNumber(), static_cast<int>(i) );

    QMailMessage m;
    m.setTo(QMailAddress("someone@example.net"));
    m.setFrom(QMailAddress("someone@example.net"));
    m.setSubject("Multiple multiparts");

    m.setMultipartType(QMailMessagePartContainer::MultipartAlternative);
    m.appendPart(p1);
    m.appendPart(p2);
    QCOMPARE( m.contentType().toString().toLower(), QByteArray("Content-Type: multipart/alternative").toLower() );
    QCOMPARE( m.transferEncoding(), QMailMessageBody::NoEncoding );
    QCOMPARE( m.partCount(), static_cast<uint>(2) );
    for (uint i = 0; i < m.partCount(); ++i)
        QCOMPARE( m.partAt(i).partNumber(), static_cast<int>(i) );

    // Test that the part locations resolve
    for (uint i = 0; i < m.partCount(); ++i) {
        const QMailMessagePart& p1 = m.partAt(i);

        QMailMessagePart::Location loc1(p1.location());
        QVERIFY(m.contains(loc1));

        const QMailMessagePart& lp1 = m.partAt(loc1);

        QCOMPARE(lp1.partNumber(), p1.partNumber());
        QCOMPARE(lp1.location().toString(true), loc1.toString(true));

        for (uint j = 0; j < p1.partCount(); ++j) {
            const QMailMessagePart& p2 = p1.partAt(j);

            QMailMessagePart::Location loc2(p2.location());
            QVERIFY(m.contains(loc2));
            const QMailMessagePart& lp2 = m.partAt(loc2);

            QCOMPARE(lp2.partNumber(), p2.partNumber());
            QCOMPARE(lp2.location().toString(true), loc2.toString(true));
        }
    }

    QByteArray rfcData = m.toRfc2822();
    QMailMessage m2 = QMailMessage::fromRfc2822(rfcData);
    QByteArray repeated = m2.toRfc2822();
    QCOMPARE(repeated, rfcData);

    QByteArray serialized;
    {
        QDataStream out(&serialized, QIODevice::WriteOnly);
        out << m;
    }
    { 
        QDataStream in(&serialized, QIODevice::ReadOnly);
        QMailMessage m3;
        in >> m3;
        repeated = m3.toRfc2822();
    }
    QCOMPARE(repeated, rfcData);

    // Test that conversion to-and-from RFC 2822 preserves all information
    QByteArray identity = m.toRfc2822(QMailMessage::IdentityFormat);

    QMailMessage m4 = QMailMessage::fromRfc2822(identity);

    QCOMPARE( m4.contentType().toString().toLower(), m.contentType().toString().toLower() );
    QCOMPARE( m4.transferEncoding(), m.transferEncoding() );
    QCOMPARE( m4.partCount(), m.partCount() );
    for (uint i = 0; i < m.partCount(); ++i) {
        const QMailMessagePart& p1 = m4.partAt(i);

        QMailMessagePart::Location loc1(p1.location());
        QVERIFY(m.contains(loc1));

        const QMailMessagePart& lp1 = m.partAt(loc1);

        QCOMPARE( p1.partNumber(), lp1.partNumber());
        QCOMPARE( p1.location().toString(true), lp1.location().toString(true));

        QCOMPARE( p1.hasBody(), lp1.hasBody());
        QCOMPARE( p1.partCount(), lp1.partCount());
        QCOMPARE( p1.body().data(), lp1.body().data());

        for (uint j = 0; j < p1.partCount(); ++j) {
            const QMailMessagePart& p2 = p1.partAt(j);

            QMailMessagePart::Location loc2(p2.location());
            QVERIFY(m.contains(loc2));

            const QMailMessagePart& lp2 = m.partAt(loc2);

            QCOMPARE( p2.partNumber(), lp2.partNumber());
            QCOMPARE( p2.location().toString(true), lp2.location().toString(true));

            QCOMPARE( p2.hasBody(), lp2.hasBody());
            QCOMPARE( p2.partCount(), lp2.partCount());
            QCOMPARE( p2.body().data(), lp2.body().data());
        }
    }
}

void tst_QMailMessage::copyAndAssign()
{
    QMailMessage m1;
    QMailMessage m2;
    QMailMessage m3(m1);

    QCOMPARE( m2.toRfc2822(), m1.toRfc2822() );
    QCOMPARE( m3.toRfc2822(), m1.toRfc2822() );

    m1.setSubject("Different");
    QVERIFY( m2.toRfc2822() != m1.toRfc2822() );
    QVERIFY( m3.toRfc2822() != m1.toRfc2822() );

    m2 = m1;
    QCOMPARE( m2.toRfc2822(), m1.toRfc2822() );

    QMailMessage m4(m1);
    QCOMPARE( m4.toRfc2822(), m1.toRfc2822() );

    m1.setBody(QMailMessageBody::fromFile("/etc/hosts", QMailMessageContentType("text/plain;\n charset=\"us-ascii\""), QMailMessageBody::SevenBit, QMailMessageBody::RequiresEncoding));
    QVERIFY( m2.toRfc2822() != m1.toRfc2822() );
    QVERIFY( m4.toRfc2822() != m1.toRfc2822() );

    m2 = m1;
    QCOMPARE( m2.toRfc2822(), m1.toRfc2822() );

    QMailMessage m5(m1);
    QCOMPARE( m5.toRfc2822(), m1.toRfc2822() );

    QMailMessageMetaData md1;
    QVERIFY( md1.subject() != m1.subject() );
    QCOMPARE( md1.serverUid(), m1.serverUid() );

    QMailMessageMetaData md2(m1);
    QCOMPARE( md2.subject(), m1.subject() );
    QCOMPARE( md2.serverUid(), m1.serverUid() );

    QMailMessageMetaData md3;
    md3 = m1;
    QCOMPARE( md3.subject(), m1.subject() );
    QCOMPARE( md3.serverUid(), m1.serverUid() );
}

void tst_QMailMessage::unterminatedDoubleQuote()
{
    QMailMessage m1;
    QString testString("\"String with unterminated double quote");
    m1.setSubject( testString );
    QMailMessage m2(QMailMessage::fromRfc2822(m1.toRfc2822()));
    QCOMPARE( m2.subject(), testString );
}
