/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#include <QObject>
#include <QTest>
#include <ctype.h>
#include <QSignalSpy>
#include <QTimer>
#include <QThread>

#include <private/qcopserver_p.h>
#include <private/qcopchannel_p.h>
#include <private/qcopchannel_p_p.h>
#include <private/qcopapplicationchannel_p.h>
#include <private/qcopadaptor_p.h>
#include <private/qcopchannelmonitor_p.h>
#include "qmailid.h"

class TestQCopServer;
class tst_QCop : public QCopAdaptor
{
    Q_OBJECT

public:
    tst_QCop() : QCopAdaptor(QString::fromLatin1("QMFTestcase")),
        server(0) {}
    virtual ~tst_QCop() {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_qcopchannel();
    void test_qcopserver();
    void test_qcopclient();
    void test_qcopadaptor();
    void test_secondqcopserver();

private:
    TestQCopServer *server;
};

class TestQCopAdaptor: public QCopAdaptor
{
    Q_OBJECT
    friend class tst_QCop;
public:
    TestQCopAdaptor(const QString &channel): QCopAdaptor(channel) {}
public slots:
    void onArg0() {}
    void onArg1(QVariant one) { Q_UNUSED(one); }
    void onArg2(QString one, QByteArray two) { Q_UNUSED(one); Q_UNUSED(two); }
    void onArg3(QString one, QString two, QString three) { Q_UNUSED(one); Q_UNUSED(two); Q_UNUSED(three); }
signals:
    void arg0();
    void arg1(QVariant one);
    void arg2(QString one, QByteArray two);
    void arg3(QString one, QString two, QString three);
};

class TestQCopServer: public QCopServer
{
    Q_OBJECT
public:
    TestQCopServer(QObject *parent = Q_NULLPTR):QCopServer(parent) {}
    qint64 activateApp(const QString &app) { return activateApplication(app); }
    void appExited(qint64 pid) { applicationExited(pid); }
};

QTEST_MAIN(tst_QCop)

#include "tst_qcop.moc"

void tst_QCop::initTestCase()
{
    server = new TestQCopServer(0);
}

void tst_QCop::cleanupTestCase()
{
    delete server;
}

void tst_QCop::test_qcopchannel()
{
    //QCopChannel
    QString channel("testchannel");
    QCopChannel cop(channel);

    QCOMPARE(cop.channel(), channel);
    QVERIFY(cop.isRegistered(channel));

    QVERIFY(QCopChannel::send(channel, "channel test1"));
    QVERIFY(QCopChannel::send(channel, "channel test2", "check for success"));
    QVERIFY(QCopChannel::flush());

    QCopChannel::reregisterAll(); // how to check?

    QSignalSpy spy1(&cop, SIGNAL(received(const QString& , const QByteArray &)));
    QCopChannel::sendLocally(channel, "channel test3", "send locally");
    QCOMPARE(spy1.count(), 1);

    //QCopApplicationChannel
    QCopApplicationChannel app;
    QVERIFY(!app.isStartupComplete()); //startup doesn't complete immediately


    //Channel Monitor
    QCopChannelMonitor monitor(channel, 0);
    QCOMPARE(monitor.channel(), channel);

    //QVERIFY(monitor.state() != QCopChannelMonitor::Unknown);
}

void tst_QCop::test_qcopserver()
{
    //QCopServerRegexp tests
    QCopLocalSocket * sock = new QCopLocalSocket;
    QCopClient *client = new QCopClient(sock, sock);
    sock->setParent(client);

    QString channel("testserverchannel");
    QCopServerRegexp regexp(channel, client);
    QVERIFY(regexp.match(channel));

    QString channel_star=channel+"*";
    QCopServerRegexp regexp1(channel_star, client);
    QVERIFY(regexp1.match(channel)); // match only the non-* part

    //QCopServer server;

    QCOMPARE(server->activateApp("invalidAppName"), qint64(-1));
    server->appExited(1391);

    client->handleRegistered(channel);
}

void tst_QCop::test_qcopclient()
{
    new QCopClient(true, this);

    QCopLocalSocket * sock = new QCopLocalSocket;
    QCopClient *client = new QCopClient(sock, sock);
    sock->setParent(client);

    QString ch("channel");
    client->handleStartupComplete(ch);
    QVERIFY(client->isStartupComplete);

    client->detach(ch);
    client->detachAll();

    QCopClient::answer(ch ,client->inBufferPtr, strlen(client->inBufferPtr));
    QCopClient::forwardLocally("5readyRead()", ch, "2startupComplete()", "");

    // todo, how to verify these functions??
    client->forward(client->inBufferPtr, ch);
    client->disconnected();
    client->reconnect();
    client->handleRegisterMonitor(ch);
    client->handleDetachMonitor(ch);
    client->handleRegistered(ch);
    client->handleUnregistered(ch);
    client->handleAck(ch);
}

void tst_QCop::test_qcopadaptor()
{
    //qcopadaptorenvelope tests
    QCopAdaptorEnvelope env;
    env << "testing const char *";

    QMailMessageId id1(123456789u);
    QMailMessageId id2(223456789u);
    QMailMessageIdList list;
    list << id1 << id2;
    env << list; // testing custom qvariant

    QCopAdaptorEnvelope env1(env);
    QCopAdaptorEnvelope env2;
    env2 = env;
    // verification?

    //adaptor tests
    QString ch("channel");
    TestQCopAdaptor adapt(ch);
    QCOMPARE(adapt.channel(), ch);

    QStringList chlist = adapt.sendChannels(ch);
    QCOMPARE(chlist.at(0), ch);

    QVERIFY(adapt.publish(SLOT(receiverDestroyed())));
    QVERIFY(adapt.publish(SIGNAL(arg0())));
    QVERIFY(!adapt.publish(SIGNAL(receiverDestroyed())));
    QVERIFY(!adapt.publish("5invalidMember()"));

    adapt.publishAll(QCopAdaptor::Signals);
    adapt.publishAll(QCopAdaptor::Slots);

    QVERIFY(adapt.isConnected(SIGNAL(receiverDestroyed())));

    connect(&adapt, SIGNAL(arg0()), &adapt, SLOT(receiverDestroyed()));
    emit adapt.arg0();

    connect(&adapt, SIGNAL(arg2(QString, QByteArray)), &adapt, SLOT(received(QString, QByteArray)));
    emit adapt.arg2("receiverDestroyed()", "argument1");

    adapt.send(SIGNAL(arg0()));
    adapt.send(SIGNAL(arg1(QVariant)), "one");
    adapt.send(SIGNAL(arg2(QString, QByteArray)), "one", "two");
    adapt.send(SIGNAL(arg3(QString, QString, QString)), "one", "two", "three");
    adapt.send(SIGNAL(arg1(QVariant)), QVariantList() << "one");

}

void tst_QCop::test_secondqcopserver()
{
    QCopLocalSocket * sock = new QCopLocalSocket;
    QCopClient *client = new QCopClient(sock, sock);
    sock->setParent(client);

    QString channel("testserverchannel");
    QCopServerRegexp regexp(channel, client);
    QVERIFY(regexp.match(channel));

    QString channel_star=channel+"*";
    QCopServerRegexp regexp1(channel_star, client);
    QVERIFY(regexp1.match(channel)); // match only the non-* part

    //QCopServer server;
    TestQCopServer *server1 = new TestQCopServer();
    QCOMPARE(server1->activateApp("invalidAppName"), qint64(-1));
    server1->appExited(1391);

    client->handleRegistered(channel);
    delete server1;
}
