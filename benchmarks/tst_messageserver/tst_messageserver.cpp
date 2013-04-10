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

#include "benchmarkcontext.h"
#include "qscopedconnection.h"
#include <imapconfiguration.h>
#include <messageserver.h>
#include <qmailnamespace.h>
#include <qmailserviceaction.h>
#include <qmailstore.h>
#include <QTest>
#include <QtCore>
#ifdef Q_OS_WIN
#else
#include <errno.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef HAVE_VALGRIND
#include "3rdparty/callgrind_p.h"
#include "3rdparty/valgrind_p.h"
#else
#define RUNNING_ON_VALGRIND 0
#define CALLGRIND_ZERO_STATS
#define CALLGRIND_DUMP_STATS
#endif

/*
    This file is $TROLLTECH_INTERNAL$
    It contains information about internal mail accounts.
*/

class tst_MessageServer;
typedef void (tst_MessageServer::*TestFunction)();

typedef QList<QByteArray> TestMail;
typedef QList<TestMail>   TestMailList;
Q_DECLARE_METATYPE(TestMailList)

class tst_MessageServer : public QObject
{
    Q_OBJECT
public:
    bool verbose;
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void completeRetrievalImap();
    void completeRetrievalImap_data();

    void removeMessages();
    void removeMessages_data();

    void replaceMessages();
    void replaceMessages_data();

protected slots:
    void onActivityChanged(QMailServiceAction::Activity);
    void onProgressChanged(uint,uint);

private:
    void completeRetrievalImap_impl();
    void removeMessages_impl();
    void replaceMessages_impl();

    void compareMessages(QMailMessageIdList const&, TestMailList const&);
    void waitForActivity(QMailServiceAction*, QMailServiceAction::Activity, int);
    void addAccount(QMailAccount*, QString const&, QString const&, QString const&, QString const&, int);
    void removePath(QString const&);
    void runInChildProcess(TestFunction);
    void runInCallgrind(QString const&);

    QEventLoop*                  m_loop;
    QTimer*                      m_timer;
    QMailServiceAction::Activity m_expectedState;
    QString                      m_imapServer;
    bool                         m_xml;
};

void tst_MessageServer::initTestCase()
{
    QProcess proc;
    proc.start("hostname -d");
    QVERIFY(proc.waitForStarted());
    QVERIFY(proc.waitForFinished());
    QByteArray out = proc.readAll();
    if (out.contains("nokia"))
        m_imapServer = "mail-nokia.trolltech.com.au";
    else
        m_imapServer = "mail.trolltech.com.au";

    m_xml = false;
    foreach (QString const& arg, QCoreApplication::arguments()) {
        if (arg == QLatin1String("-xml") || arg == QLatin1String("-lightxml")) {
            m_xml = true;
        }
    }
}

void tst_MessageServer::cleanupTestCase()
{
}

void tst_MessageServer::init()
{
    removePath(QMail::dataPath());
}

void tst_MessageServer::cleanup()
{ init(); }

void tst_MessageServer::removePath(QString const& path)
{
    QFileInfo fi(path);
    if (!fi.exists()) return;

    if (fi.isDir() && !fi.isSymLink()) {
        QDir dir(path);
        foreach (QString const& name, dir.entryList(QDir::NoDotAndDotDot|QDir::AllEntries|QDir::Hidden)) {
            removePath(path + '/' + name);
        }
    }

    QDir parent = fi.dir();
    QString filename = fi.fileName();

    if (!filename.isEmpty()) {
        bool ok;
        if (fi.isDir() && !fi.isSymLink()) {
            ok = parent.rmdir(filename);
        }
        else {
            ok = parent.remove(filename);
        }
        if (!ok) {
            qFatal("Could not delete %s", qPrintable(path));
        }
    }
}

void tst_MessageServer::completeRetrievalImap()
{
    runInChildProcess(&tst_MessageServer::completeRetrievalImap_impl);
    if (QTest::currentTestFailed()) return;

    QByteArray tag = QTest::currentDataTag();
    if (tag == "small_messages--100" || tag == "small_messages--500") {
        runInCallgrind("completeRetrievalImap:" + tag);
    }
}

void tst_MessageServer::runInCallgrind(QString const& testfunc)
{
    if (RUNNING_ON_VALGRIND) return;

    /*
        Run a particular testfunc in a separate process under callgrind.
    */
    QString thisapp = QCoreApplication::applicationFilePath();

    // Strip any testfunction args
    QMetaObject const* mo = metaObject();
    QStringList testfunctions;
    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod mm = mo->method(i);
        if (mm.methodType() == QMetaMethod::Slot && mm.access() == QMetaMethod::Private) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            QByteArray sig = mm.methodSignature();
#else
            QByteArray sig = mm.signature();
#endif
            testfunctions << QString::fromLatin1(sig.left(sig.indexOf('(')));
        }
    }
    QStringList args;
    foreach (QString const& arg, QCoreApplication::arguments()) {
        bool bad = false;
        foreach (QString const& tf, testfunctions) {
            if (arg.startsWith(tf)) {
                bad = true;
                break;
            }
        }
        if (!bad) args << arg;
    }
    args.removeAt(0); // application name

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    proc.start("valgrind", QStringList()
        << "--tool=callgrind"
        << "--quiet"
        << "--"
        << thisapp
        << args
        << testfunc
    );

    if (!proc.waitForStarted(30000)) {
        QFAIL(qPrintable(QString("Failed to start %1 under callgrind: %2").arg(testfunc)
            .arg(proc.errorString())));
    }

    static const int timeoutSeconds = 60*60;
    if (!proc.waitForFinished(timeoutSeconds*1000)) {
        QFAIL(qPrintable(QString("%1 in callgrind didn't finish within %2 seconds\n"
            "Output:\n%3")
            .arg(testfunc)
            .arg(timeoutSeconds)
            .arg(QString::fromLocal8Bit(proc.readAll()))
        ));
    }

    if (proc.exitStatus() != QProcess::NormalExit) {
        QFAIL(qPrintable(QString("%1 in callgrind crashed: %2\n"
            "Output:\n%3")
            .arg(testfunc)
            .arg(proc.errorString())
            .arg(QString::fromLocal8Bit(proc.readAll()))
        ));
    }

    if (proc.exitCode() != 0) {
        QFAIL(qPrintable(QString("%1 in callgrind exited with code %2\n"
            "Output:\n%3")
            .arg(testfunc)
            .arg(proc.exitCode())
            .arg(QString::fromLocal8Bit(proc.readAll()))
        ));
    }

    /*
        OK, it ran fine.  Now reproduce the benchmark lines exactly as they appeared in
        the child process.
    */
    foreach (QByteArray const& line, proc.readAll().split('\n')) {
        if (line.contains("callgrind")) {
            fprintf(stdout, "%s\n", line.constData());
        }
    }
}

void tst_MessageServer::runInChildProcess(TestFunction fn)
{
#if defined(Q_OS_WIN)
	// No advantage to forking?
    (this->*fn)();
#else
    if (RUNNING_ON_VALGRIND) {
        qWarning(
            "Test is being run under valgrind. Testfunctions will not be run in child processes.\n"
            "Run only one testfunction per test run for best results.\n"
        );
        (this->*fn)();
        return;
    }

    /*
        Run the test in a separate process.
        This is done so that subsequent tests are not affected by any in-process
        caching of data or left over data from a previous run.
    */
    pid_t pid = ::fork();
    if (-1 == pid) {
        qFatal("fork: %s", strerror(errno));
    }
    if (0 != pid) {
        int status;
        if (pid != waitpid(pid, &status, 0))
            qFatal("waitpid: %s", strerror(pid));
        if (!WIFEXITED(status)) {
            if (WIFSIGNALED(status)) {
                status = WTERMSIG(status);
                QFAIL(qPrintable(QString("Child terminated by signal %1").arg(status)));
            }
            QFAIL(qPrintable(QString("Child exited for unknown reason with status %1").arg(status)));
        }

        status = WEXITSTATUS(status);
        if (status != 0) {
            QFAIL(qPrintable(QString("Child exited with exit code %1").arg(status)));
        }
        return;
    }

    /* We get these messages from not using a QtopiaApplication */
    for (int i = 0; i < 2; ++i) {
        QTest::ignoreMessage(QtWarningMsg, "Object::connect: No such signal QApplication::appMessage(QString,QByteArray)");
        QTest::ignoreMessage(QtWarningMsg, "Object::connect:  (sender name:   'tst_messageserver')");
    }

    (this->*fn)();

    int exitcode = 0;
    if (QTest::currentTestFailed())
        exitcode = 1;

    fflush(stdout);
    fflush(stderr);
    _exit(exitcode);
#endif
}

/* Test full retrieval of all messages from a specific account */
void tst_MessageServer::completeRetrievalImap_impl()
{
    static const char service[] = "imap4";

    QFETCH(QString, user);
    QFETCH(QString, password);
    QFETCH(QString, server);
    QFETCH(int,     port);
    QFETCH(TestMailList,mails);

    QMailMessageIdList fetched;
    /* Valgrind slows things down quite a lot. */
    static const int MAXTIME =
        RUNNING_ON_VALGRIND ? qMax(60000, 2000*mails.count()) :
        60000;

    QMailStore* ms = 0;
    {
        BenchmarkContext ctx(m_xml);

        new MessageServer;
        ms = QMailStore::instance();

        QMailAccount account;
        addAccount(&account, service, user, password, server, port);
        if (QTest::currentTestFailed()) return;

        /* Get message count for this account */
        QMailRetrievalAction retrieve;

        retrieve.synchronizeAll(account.id());
        waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
        if (QTest::currentTestFailed()) return;

        /* Ensure we have all the messages we expect */
        QCOMPARE(ms->countMessages(), mails.count());

        /* OK, now download the entire message bodies. */
        fetched = ms->queryMessages();
        QCOMPARE(fetched.count(), mails.count());
        retrieve.retrieveMessages(fetched, QMailRetrievalAction::Content);
        waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
        if (QTest::currentTestFailed()) return;
    }

    compareMessages(fetched, mails);
}

void tst_MessageServer::waitForActivity(QMailServiceAction* action, QMailServiceAction::Activity state, int timeout)
{
    QScopedConnection c1(action, SIGNAL(activityChanged(QMailServiceAction::Activity)), this, SLOT(onActivityChanged(QMailServiceAction::Activity)));
    QScopedConnection c2(action, SIGNAL(progressChanged(uint,uint)), this, SLOT(onProgressChanged(uint,uint)));

    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.setInterval(timeout);
    timer.setSingleShot(true);

    m_expectedState = state;

    timer.start();

    m_loop = &loop;
    m_timer = &timer;
    int code = loop.exec();
    bool timed_out = m_loop;
    m_timer = 0;
    m_loop = 0;

    QCOMPARE(code, 0);

    QVERIFY2(!timed_out, qPrintable(QString("%1 timed out").arg(QString::fromLatin1(action->metaObject()->className()))));
}

void tst_MessageServer::addAccount(QMailAccount* account, QString const& service, QString const& user, QString const& password, QString const& server, int port)
{
    if (service != QLatin1String("imap4")) {
        QFAIL(qPrintable(QString("Unknown service type %1").arg(service)));
    }

    account->setMessageType(QMailMessageMetaData::Email);
    account->setStatus(QMailAccount::CanRetrieve, true);
    account->setStatus(QMailAccount::MessageSource, true);
    account->setStatus(QMailAccount::Enabled, true);

    QMailAccountConfiguration config;
    config.addServiceConfiguration(service);

    ImapConfigurationEditor imap(&config);
    imap.setVersion(100);
    imap.setType(QMailServiceConfiguration::Source);
    imap.setMailUserName(user);
    imap.setMailPassword(password);
    imap.setMailServer(server);
    imap.setMailPort(port);
    imap.setAutoDownload(false);
    imap.setDeleteMail(false);
    imap.setMaxMailSize(0);

    QVERIFY(QMailStore::instance()->addAccount(account, &config));
}

void tst_MessageServer::compareMessages(QMailMessageIdList const& actual, TestMailList const& expected)
{
    /*
        Go through the fetched messages and make sure they are what we expect.
        Note that this should be outside of BenchmarkContext sections so we don't count the
        memory/time used to do this.
    */
    QMailStore* ms = QMailStore::instance();
    for (int i = 0; i < actual.count(); ++i) {
        QByteArray act = ms->message(actual.at(i)).toRfc2822();
        QVERIFY(act.size() > 0);
        foreach (QByteArray const& exp, expected.at(i)) {
            QVERIFY2(act.contains(exp), qPrintable(QString("Message was expected to contain this string, but didn't: %1\nMessage: %2").arg(QString::fromLatin1(exp)).arg(QString::fromLatin1(act))));
        }
#ifdef LEARN
        if (!exp.at(i).count()) {
            foreach (QByteArray const& line, actual.split('\n'))
                qDebug() << line.constData(); // not compiled by default
        }
#endif
    }
}

void tst_MessageServer::completeRetrievalImap_data()
{
    QTest::addColumn<QString>     ("user");
    QTest::addColumn<QString>     ("password");
    QTest::addColumn<QString>     ("server");
    QTest::addColumn<int>         ("port");
    QTest::addColumn<TestMailList>("mails");

    /*
        Note - this testdata is deliberately _not_ strictly in order from smallest
        to largest, because if it were, resource leaks between tests might be hidden.
    */

    TestMailList list;

    list.clear();
    for (int i = 0; i < 200; ++i)
        list << TestMail();
    QTest::newRow("small_messages--200")
        << QString::fromLatin1("mailtst31")
        << QString::fromLatin1("testme31")
        << m_imapServer
        << 143
        << list
    ;

    list.clear();
    for (int i = 0; i < 1000; ++i)
        list << TestMail();
    QTest::newRow("small_messages--1000")
        << QString::fromLatin1("mailtst33")
        << QString::fromLatin1("testme33")
        << m_imapServer
        << 143
        << list
    ;
    list.clear();
    for (int i = 0; i < 100; ++i)
        list << TestMail();
    QTest::newRow("small_messages--100")
        << QString::fromLatin1("mailtst30")
        << QString::fromLatin1("testme30")
        << m_imapServer
        << 143
        << list
    ;

    QTest::newRow("big_messages")
        << QString::fromLatin1("mailtst37")
        << QString::fromLatin1("testme37")
        << m_imapServer
        << 143
        << (TestMailList()
            << (TestMail()
                << QByteArray("Subject: 4MB+ test file")
                << QByteArray("o+ZmfhB18O/FYfHMEspiVR3/nRPYBXfnCyLURiIRyM0Lx7bXk9MtpRPEnL01xiAkeBobLd/e2ZKb")
                << QByteArray("YMSd7zfs2xNPsCNwj76/73/Vx9SSJ//1RIyxewgR//4u5IpwoSEWMp5+")
            )
            << (TestMail()
                << QByteArray("Subject: 20,000 Leagues Under the Sea")
                << QByteArray("I went to the central staircase which opened on to the platform,")
                << QByteArray("from Ceylon to Sydney, touching at King George's Point and Melbourne.")
                << QByteArray("274     2  occurred                occurred")
            )
            << (TestMail()
                << QByteArray("Subject: Fwd:3: Message with multiple attachments")
                << QByteArray("SUsTXEe2aBGuTYOxhAH0og014xzV2R147zXEdIH1robUCKcaYTgmZ96lMWSGXN6sq+KmpMKIAx8V")
                << QByteArray("Content-Type: image/png; name=snapshot2.png")
                << QByteArray("RsySH338IAAGE+jk306+9ONT2nhj5rNnp44+f/S1s2888fijVl/5zE/PVO+pnvq7U0f+6ggrHH56")
                << QByteArray("Content-Type: image/jpeg; name=snapshot11.jpg")
                << QByteArray("CUAAAAAAPWYXQrHZ0+CZDhdCsdnT4JlGwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")
            )
        )
    ;

    list.clear();
    for (int i = 0; i < 500; ++i)
        list << TestMail();
    QTest::newRow("small_messages--500")
        << QString::fromLatin1("mailtst32")
        << QString::fromLatin1("testme32")
        << m_imapServer
        << 143
        << list
    ;

    list.clear();
    for (int i = 0; i < 2000; ++i)
        list << TestMail();
    QTest::newRow("small_messages--2000")
        << QString::fromLatin1("mailtst34")
        << QString::fromLatin1("testme34")
        << m_imapServer
        << 143
        << list
    ;

#if VERY_PATIENT_TESTER
    list.clear();
    for (int i = 0; i < 5000; ++i)
        list << TestMail();
    QTest::newRow("small_messages--5000")
        << QString::fromLatin1("mailtst35")
        << QString::fromLatin1("testme35")
        << m_imapServer
        << 143
        << list
    ;

    list.clear();
    for (int i = 0; i < 10000; ++i)
        list << TestMail();
    QTest::newRow("small_messages--10000")
        << QString::fromLatin1("mailtst36")
        << QString::fromLatin1("testme36")
        << m_imapServer
        << 143
        << list
    ;
#endif
}

void tst_MessageServer::removeMessages()
{ runInChildProcess(&tst_MessageServer::removeMessages_impl); }

void tst_MessageServer::removeMessages_impl()
{
    static const char service[] = "imap4";

    QFETCH(QString, user);
    QFETCH(QString, password);
    QFETCH(QString, server);
    QFETCH(int,     port);
    QFETCH(TestMailList,mails);

    QMailMessageIdList fetched;
    /* Valgrind slows things down quite a lot. */
    static const int MAXTIME =
        RUNNING_ON_VALGRIND ? qMax(60000, 2000*mails.count()) :
        60000;

    new MessageServer;
    QMailStore* ms = QMailStore::instance();

    QMailAccount account;
    addAccount(&account, service, user, password, server, port);
    if (QTest::currentTestFailed()) return;

    /* Get message count for this account */
    QMailRetrievalAction retrieve;

    retrieve.synchronizeAll(account.id());
    waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
    if (QTest::currentTestFailed()) return;

    /* Ensure we have all the messages we expect */
    QCOMPARE(ms->countMessages(), mails.count());

    /* OK, now download the entire message bodies. */
    fetched = ms->queryMessages();
    QCOMPARE(fetched.count(), mails.count());
    retrieve.retrieveMessages(fetched, QMailRetrievalAction::Content);
    waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
    if (QTest::currentTestFailed()) return;

    compareMessages(fetched, mails);
    if (QTest::currentTestFailed()) return;

    {
        BenchmarkContext ctx(m_xml);
        QVERIFY(ms->removeMessages(QMailMessageKey(), QMailStore::NoRemovalRecord));
    }

    QCOMPARE(ms->queryMessages().count(), 0);
}

void tst_MessageServer::removeMessages_data()
{ completeRetrievalImap_data(); }

void tst_MessageServer::replaceMessages()
{ runInChildProcess(&tst_MessageServer::replaceMessages_impl); }

/*
    Tests that downloading, deleting and redownloading the same mails does
    not leak filesystem resources.
    This test ensures the sqlite database is correctly reusing the space
    freed when removing mails.
*/
void tst_MessageServer::replaceMessages_impl()
{
    static const char service[] = "imap4";

    QFETCH(QString, user);
    QFETCH(QString, password);
    QFETCH(QString, server);
    QFETCH(int,     port);
    QFETCH(TestMailList,mails);

    QMailMessageIdList fetched;
    /* Valgrind slows things down quite a lot. */
    static const int MAXTIME =
        RUNNING_ON_VALGRIND ? qMax(60000, 2000*mails.count()) :
        60000;

    new MessageServer;
    QMailStore* ms = QMailStore::instance();

    QMailAccount account;
    addAccount(&account, service, user, password, server, port);
    if (QTest::currentTestFailed()) return;

    /* Get message count for this account */
    QMailRetrievalAction retrieve;

    retrieve.synchronizeAll(account.id());
    waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
    if (QTest::currentTestFailed()) return;

    /* Ensure we have all the messages we expect */
    QCOMPARE(ms->countMessages(), mails.count());

    /* OK, now download the entire message bodies. */
    fetched = ms->queryMessages();
    QCOMPARE(fetched.count(), mails.count());
    retrieve.retrieveMessages(fetched, QMailRetrievalAction::Content);
    waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
    if (QTest::currentTestFailed()) return;

    compareMessages(fetched, mails);
    if (QTest::currentTestFailed()) return;

    {
        /* Remove the messages. */
        BenchmarkContext ctx(m_xml);
        QVERIFY(ms->removeMessages(QMailMessageKey(), QMailStore::NoRemovalRecord));
        QCOMPARE(ms->queryMessages().count(), 0);

        /* Redownload the same messages. */
        retrieve.synchronizeAll(account.id());
        waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
        if (QTest::currentTestFailed()) return;

        /* Ensure we have all the messages we expect */
        QCOMPARE(ms->countMessages(), mails.count());

        /* OK, now download the entire message bodies. */
        fetched = ms->queryMessages();
        QCOMPARE(fetched.count(), mails.count());
        retrieve.retrieveMessages(fetched, QMailRetrievalAction::Content);
        waitForActivity(&retrieve, QMailServiceAction::Successful, MAXTIME);
        if (QTest::currentTestFailed()) return;
    }

    compareMessages(fetched, mails);
}

void tst_MessageServer::replaceMessages_data()
{ completeRetrievalImap_data(); }


void tst_MessageServer::onActivityChanged(QMailServiceAction::Activity a)
{
    if (!m_loop) return;

    /*
        Exit the inner loop with success if we got the expected state, failure
        if we got a failure state.
        Set m_loop = 0 so it can be determined we didn't time out.
    */
    if (a == m_expectedState) {
        m_loop->exit(0);
        m_loop = 0;
    }
    else if (a == QMailServiceAction::Failed) {
        m_loop->exit(1);
        m_loop = 0;
    }
}

void tst_MessageServer::onProgressChanged(uint value,uint total)
{
    /*
        Running in valgrind takes a long time... output some progress so it's
        clear we haven't frozen.
    */
    static int i = 0;
    bool output = RUNNING_ON_VALGRIND || verbose;
    if (output && !(i++ % 25)) {
        qWarning() << "Progress:" << value << '/' << total;
    }

    /* We are making some progress, so reset the timeout timer. */
    if (m_timer) {
        m_timer->start();
    }
}

int main(int argc, char** argv)
{
    /*
        Ensure that this test doesn't run as the QWS server.
        This is because we intend to fork() later on and we don't want to
        have to process events for the sake of the child process.
    */
    QApplication app(argc, argv);

    int iters = 1;
    bool verbose = false;
    for (int i = 0; i < argc; ++i) {
        if (i < argc-1 && !strcmp(argv[i], "-iterations")) {
            bool ok;
            int n = QString::fromLatin1(argv[i+1]).toInt(&ok);
            if (ok && n > 0) iters = n;
        }
        else if (!strncmp(argv[i], "-v", 2)) {
            verbose = true;
        }
    }

    int ret = 0;
    for (int i = 0; i < iters; ++i) {
        tst_MessageServer test;
        test.verbose = verbose;
        ret += QTest::qExec(&test, argc, argv);
    }
    return ret;
}

#include "tst_messageserver.moc"

