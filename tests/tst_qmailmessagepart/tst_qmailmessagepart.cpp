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
#include <qmailmessage.h>

/*
Note: Any email addresses appearing in this test data must be example addresses,
as defined by RFC 2606.  Therefore, they should use one of the following domains:
    *.example.{com|org|net}
    *.test
    *.example
*/

// RFC 2822 messages use CRLF as the newline indicator
#define CRLF "\015\012"

// Override the boundary string to yield predictable results
#define BOUNDARY "[}<}]"
void setQMailMessageBoundaryString(const QByteArray &boundary);

//TESTED_CLASS=QMailMessagePart
//TESTED_FILES=src/libraries/qtopiamail/qmailmessage.cpp

/*
    Unit test for QMailMessagePart class.
    This class primarily tests that QMailMessagePart correctly sets/gets properties.
*/
class tst_QMailMessagePart : public QObject
{
    Q_OBJECT

public:
    tst_QMailMessagePart();
    virtual ~tst_QMailMessagePart();

private slots:
    void contentID();
    void setContentID();
    void contentLocation();
    void setContentLocation();
    void contentDescription();
    void setContentDescription();
    void contentDisposition();
    void setContentDisposition();
    void contentLanguage();
    void setContentLanguage();

    void headerField();
    void headerFieldText();
    void headerFields();
    void headerFieldsText();

    void setHeaderField();
    void appendHeaderField();
    void removeHeaderField();

    void testSerialization();
};

QTEST_MAIN(tst_QMailMessagePart)
#include "tst_qmailmessagepart.moc"


tst_QMailMessagePart::tst_QMailMessagePart()
{
    setQMailMessageBoundaryString(BOUNDARY);
}

tst_QMailMessagePart::~tst_QMailMessagePart()
{
    setQMailMessageBoundaryString(QByteArray());
}

void tst_QMailMessagePart::contentID()
{
    // Tested-by: setContentID
}

void tst_QMailMessagePart::setContentID()
{
    QMailMessagePart part;

    QString id1("Some content-ID x@yyyy");
    QString id2("<Some other content-ID y@yyyy>");

    QCOMPARE( part.contentID(), QString() );

    part.setContentID(id1);
    QCOMPARE( part.contentID(), id1 );

    part.setContentID(id2);
    QCOMPARE( part.contentID(), id2.mid(1, id2.length() - 2) );
}

void tst_QMailMessagePart::contentLocation()
{
    // Tested-by: setContentLocation
}

void tst_QMailMessagePart::setContentLocation()
{
    QMailMessagePart part;

    QString location1("Some content-location");
    QString location2("Some other content-location");

    QCOMPARE( part.contentLocation(), QString() );

    part.setContentLocation(location1);
    QCOMPARE( part.contentLocation(), location1 );

    part.setContentLocation(location2);
    QCOMPARE( part.contentLocation(), location2 );
}

void tst_QMailMessagePart::contentDescription()
{
    // Tested-by: setContentDescription
}

void tst_QMailMessagePart::setContentDescription()
{
    QMailMessagePart part;

    QString description1("Some content-description");
    QString description2("Some other content-description");

    QCOMPARE( part.contentDescription(), QString() );

    part.setContentDescription(description1);
    QCOMPARE( part.contentDescription(), description1 );

    part.setContentDescription(description2);
    QCOMPARE( part.contentDescription(), description2 );
}

void tst_QMailMessagePart::contentDisposition()
{
    // Tested-by: setContentDisposition
}

void tst_QMailMessagePart::setContentDisposition()
{
    QMailMessagePart part;

    QByteArray disposition1("Content-Disposition: inline");
    QByteArray disposition2("Content-Disposition: attachment; filename=sample.txt");

    QCOMPARE( part.contentDisposition().toString(), QMailMessageContentDisposition().toString() );

    part.setContentDisposition(QMailMessageContentDisposition(disposition1));
    QCOMPARE( part.contentDisposition().toString(), QMailMessageContentDisposition(disposition1).toString() );

    part.setContentDisposition(QMailMessageContentDisposition(disposition2));
    QCOMPARE( part.contentDisposition().toString(), QMailMessageContentDisposition(disposition2).toString() );
}

void tst_QMailMessagePart::contentLanguage()
{
    // Tested-by: setContentLanguage
}

void tst_QMailMessagePart::setContentLanguage()
{
    QMailMessagePart part;

    QString language1("en");
    QString language2("de");

    QCOMPARE( part.contentLanguage(), QString() );

    part.setContentLanguage(language1);
    QCOMPARE( part.contentLanguage(), language1 );

    part.setContentLanguage(language2);
    QCOMPARE( part.contentLanguage(), language2 );
}

void tst_QMailMessagePart::headerField()
{
    // Tested by: setHeaderField
}

void tst_QMailMessagePart::headerFieldText()
{
    // Tested by: setHeaderField
}

void tst_QMailMessagePart::headerFields()
{
    // Tested by: appendHeaderField
}

void tst_QMailMessagePart::headerFieldsText()
{
    // Tested by: appendHeaderField
}

void tst_QMailMessagePart::setHeaderField()
{
    QString addr1("bob@example.com");
    QString addr2("jim@example.org");
    QString ownHdr("hello");

    QMailMessage m;
    m.setHeaderField("To", addr2);
    QCOMPARE(m.headerFieldText("to"), addr2);
    QCOMPARE(m.headerField("to").content(), addr2.toLatin1());

    // Ensure overwrite
    m.setHeaderField("To", addr1);
    m.setHeaderField("X-My-Own-Header", ownHdr);
    QCOMPARE(m.headerFieldText("to"), addr1);
    QCOMPARE(m.headerField("to").content(), addr1.toLatin1());
    QCOMPARE(m.headerFieldText("X-My-Own-Header"), ownHdr);
    QCOMPARE(m.headerField("X-My-Own-Header").content(), ownHdr.toLatin1());
    QCOMPARE(m.to(), (QList<QMailAddress>() << QMailAddress(addr1)));

    QCOMPARE(m.recipients(), (QList<QMailAddress>() << QMailAddress(addr1)));
    QMailMessageMetaData mtdata = *static_cast<QMailMessageMetaData*>(&m);
    QCOMPARE(mtdata.recipients(), (QList<QMailAddress>() << QMailAddress(addr1)));
    m.setHeaderField("Cc", addr2);
    QCOMPARE(m.recipients(), (QList<QMailAddress>() << QMailAddress(addr1) << QMailAddress(addr2)));
    mtdata = *static_cast<QMailMessageMetaData*>(&m);
    QCOMPARE(mtdata.recipients(), (QList<QMailAddress>() << QMailAddress(addr1) << QMailAddress(addr2)));
    QCOMPARE(m.cc(), (QList<QMailAddress>()  << QMailAddress(addr2)));
    QString addr3("john@example.org");
    m.setHeaderField("Bcc", addr3);
    QCOMPARE(m.recipients(), (QList<QMailAddress>() << QMailAddress(addr1) << QMailAddress(addr2) << QMailAddress(addr3)));
    mtdata = *static_cast<QMailMessageMetaData*>(&m);
    QCOMPARE(mtdata.recipients(), (QList<QMailAddress>() << QMailAddress(addr1) << QMailAddress(addr2) << QMailAddress(addr3)));
    QCOMPARE(m.bcc(), (QList<QMailAddress>()  << QMailAddress(addr3)));

    QString rfc822 = m.toRfc2822();

    QMailMessage m2 = QMailMessage::fromRfc2822(rfc822.toLatin1());
    QCOMPARE(m2.headerFieldText("to"), addr1);
    QCOMPARE(m2.headerField("to").content(), addr1.toLatin1());
    QCOMPARE(m2.headerFieldText("X-My-Own-Header"), ownHdr);
    QCOMPARE(m2.headerField("X-My-Own-Header").content(), ownHdr.toLatin1());
    QCOMPARE(m2.to(), (QList<QMailAddress>() << QMailAddress(addr1)));

    m2.setTo(QList<QMailAddress>() << QMailAddress(addr2));
    QCOMPARE(m2.headerFieldText("to"), addr2);
    QCOMPARE(m2.headerField("to").content(), addr2.toLatin1());
    QCOMPARE(m2.to(), (QList<QMailAddress>() << QMailAddress(addr2)));
}

void tst_QMailMessagePart::appendHeaderField()
{
    QString addr1("bob@example.com");
    QString addr2("jim@example.org");

    QMailMessage m;
    QCOMPARE(m.headerFieldText("Resent-From"), QString());
    QCOMPARE(m.headerField("Resent-From"), QMailMessageHeaderField());
    QCOMPARE(m.headerFieldsText("Resent-From"), QStringList());
    QCOMPARE(m.headerFields("Resent-From"), QList<QMailMessageHeaderField>());

    m.appendHeaderField("Resent-From", addr1);
    QCOMPARE(m.headerFieldText("Resent-From"), addr1);
    QCOMPARE(m.headerField("Resent-From").content(), addr1.toLatin1());
    QCOMPARE(m.headerFieldsText("Resent-From"), QStringList(addr1));
    QCOMPARE(m.headerFields("Resent-From"), ( QList<QMailMessageHeaderField>() 
                                                 << QMailMessageHeaderField("Resent-From", addr1.toLatin1()) ) );

    m.appendHeaderField("Resent-From", addr2);
    QCOMPARE(m.headerFieldText("Resent-From"), addr1);
    QCOMPARE(m.headerField("Resent-From").content(), addr1.toLatin1());
    QCOMPARE(m.headerFieldsText("Resent-From"), (QStringList() << addr1 << addr2));
    QCOMPARE(m.headerFields("Resent-From"), ( QList<QMailMessageHeaderField>() 
                                                 << QMailMessageHeaderField("Resent-From", addr1.toLatin1())
                                                 << QMailMessageHeaderField("Resent-From", addr2.toLatin1()) ) );
}

void tst_QMailMessagePart::removeHeaderField()
{
    QString addr1("bob@example.com");
    QString addr2("jim@example.org");

    QMailMessage m;
    QCOMPARE(m.headerFieldText("Resent-From"), QString());
    QCOMPARE(m.headerField("Resent-From"), QMailMessageHeaderField());
    QCOMPARE(m.headerFieldsText("Resent-From"), QStringList());
    QCOMPARE(m.headerFields("Resent-From"), QList<QMailMessageHeaderField>());

    m.appendHeaderField("Resent-From", addr1);
    m.appendHeaderField("Resent-From", addr2);
    QCOMPARE(m.headerFieldText("Resent-From"), addr1);
    QCOMPARE(m.headerField("Resent-From").content(), addr1.toLatin1());
    QCOMPARE(m.headerFieldsText("Resent-From"), (QStringList() << addr1 << addr2));
    QCOMPARE(m.headerFields("Resent-From"), ( QList<QMailMessageHeaderField>() 
                                                 << QMailMessageHeaderField("Resent-From", addr1.toLatin1())
                                                 << QMailMessageHeaderField("Resent-From", addr2.toLatin1()) ) );

    m.removeHeaderField("X-Unused-Header");
    QCOMPARE(m.headerFieldText("Resent-From"), addr1);
    QCOMPARE(m.headerField("Resent-From").content(), addr1.toLatin1());
    QCOMPARE(m.headerFieldsText("Resent-From"), (QStringList() << addr1 << addr2));
    QCOMPARE(m.headerFields("Resent-From"), ( QList<QMailMessageHeaderField>() 
                                                 << QMailMessageHeaderField("Resent-From", addr1.toLatin1())
                                                 << QMailMessageHeaderField("Resent-From", addr2.toLatin1()) ) );

    m.removeHeaderField("Resent-From");
    QCOMPARE(m.headerFieldText("Resent-From"), QString());
    QCOMPARE(m.headerField("Resent-From"), QMailMessageHeaderField());
    QCOMPARE(m.headerFieldsText("Resent-From"), QStringList());
    QCOMPARE(m.headerFields("Resent-From"), QList<QMailMessageHeaderField>());
}

void tst_QMailMessagePart::testSerialization()
{
    QByteArray data;
    QByteArray type;

    type = "text/plain; charset=us-ascii";
    data = "P1: This is a plain text part.";
    QMailMessagePart p1;
    p1.setBody(QMailMessageBody::fromData(data, QMailMessageContentType(type), QMailMessageBody::SevenBit, QMailMessageBody::RequiresEncoding));
    p1.setContentID("P1");
    p1.setContentLocation("After the header");
    p1.setContentDescription("The first part");
    QCOMPARE( p1.contentType().toString().toLower(), QByteArray("Content-Type: text/plain; charset=us-ascii").toLower() );
    QCOMPARE( p1.transferEncoding(), QMailMessageBody::SevenBit );

    type = "text/html; charset=UTF-8";
    data = "<html>P2: This is a HTML part</html>";
    QMailMessageContentType ct(type);
    ct.setName("P2");
    QMailMessageContentDisposition cd(QMailMessageContentDisposition::Inline);
    cd.setFilename("p2.html");
    QMailMessagePart p2;
    p2.setBody(QMailMessageBody::fromData(data, ct, QMailMessageBody::EightBit, QMailMessageBody::RequiresEncoding));
    p2.setContentDisposition(cd);
    QCOMPARE( p2.contentType().toString().toLower(), QByteArray("Content-Type: text/html; charset=UTF-8; name=P2").toLower() );
    QCOMPARE( p2.transferEncoding(), QMailMessageBody::EightBit );

    QMailMessage m;
    m.setTo(QMailAddress("someone@example.net"));
    m.setFrom(QMailAddress("someone-else@example.net"));
    m.setDate(QMailTimeStamp("Fri, 22 Jun 2007 11:34:47 +1000"));

    m.setMultipartType(QMailMessagePartContainer::MultipartAlternative);
    m.appendPart(p1);
    m.appendPart(p2);
    QCOMPARE( m.contentType().toString().toLower(), QByteArray("Content-Type: multipart/alternative").toLower() );
    QCOMPARE( m.transferEncoding(), QMailMessageBody::NoEncoding );

    const QByteArray expected(
"To: someone@example.net" CRLF
"From: someone-else@example.net" CRLF
"Date: Fri, 22 Jun 2007 11:34:47 +1000" CRLF
"Content-Type: multipart/alternative; boundary=\"" BOUNDARY "\"" CRLF
"MIME-Version: 1.0" CRLF
CRLF
"This is a multipart message in Mime 1.0 format" CRLF
CRLF
"--" BOUNDARY CRLF
"Content-Type: text/plain; charset=us-ascii" CRLF
"Content-Transfer-Encoding: 7bit" CRLF
"Content-ID: <P1>" CRLF
"Content-Location: After the header" CRLF
"Content-Description: The first part" CRLF
CRLF
"P1: This is a plain text part." CRLF
"--" BOUNDARY CRLF
"Content-Type: text/html; charset=utf-8; name=P2" CRLF
"Content-Transfer-Encoding: 8bit" CRLF
"Content-Disposition: inline; filename=p2.html" CRLF
CRLF
"<html>P2: This is a HTML part</html>" CRLF
"--" BOUNDARY "--" CRLF
);

    QByteArray serialized = m.toRfc2822();
    QCOMPARE( serialized, expected );

    QMailMessage m2 = QMailMessage::fromRfc2822(serialized);
    QByteArray repeat = m.toRfc2822();
    QCOMPARE( serialized, repeat );
}

