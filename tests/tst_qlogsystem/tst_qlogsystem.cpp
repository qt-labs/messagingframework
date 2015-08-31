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
#include <QFile>

#include <qloggers.h>

//TESTED_CLASS= LogSystem
//TESTED_FILES=src/libraries/qtopiamail/support/qlogsystem.cpp

/*
    This class primarily tests that LogSystem correctly handles e-mail messages.
*/
class tst_QLogSystem : public QObject
{
    Q_OBJECT

public:
    tst_QLogSystem();
    virtual ~tst_QLogSystem();

private slots:
    virtual void initTestCase();
    virtual void cleanupTestCase();
    virtual void init();
    virtual void cleanup();

    void loggerConstruction();
    void minLogLvl();

    void loggerOutput_data();
    void loggerOutput();

    void qDebugOutput_data();
    void qDebugOutput();

    void qWarningOutput_data();
    void qWarningOutput();

    void sysLoggerOutput();
};

QTEST_MAIN(tst_QLogSystem)

#include "tst_qlogsystem.moc"


tst_QLogSystem::tst_QLogSystem()
{
}

tst_QLogSystem::~tst_QLogSystem()
{
}

void tst_QLogSystem::initTestCase()
{ // this file will be regenerated in each test
    if(QFile::exists("./LoggersTest.log"))
        QVERIFY2(QFile::remove("./LoggersTest.log"), "could not delete log file");
}


void tst_QLogSystem::cleanupTestCase()
{
}

void tst_QLogSystem::init()
{
}

void tst_QLogSystem::cleanup()
{
    if(QFile::exists("./LoggersTest.log"))
        QVERIFY2(QFile::remove("./LoggersTest.log"), "could not delete log file");
}
Q_DECLARE_METATYPE(LogLevel)

void tst_QLogSystem::loggerConstruction()
{
    FileLogger<> l_name1("NO-DIRECTORY/file-should-not-be-opened");
    QString err_msg;
    QVERIFY2(!l_name1.isReady(err_msg), "File logger is ready with invalid dir.");
    QVERIFY2(!err_msg.isEmpty(), "File logger has not obtained error message");

    err_msg = QString("");

    FileLogger<> l_name2("./LoggersTest.log");
    QVERIFY2(l_name2.isReady(err_msg), "File logger is not ready with local dir.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with local dir");
    QVERIFY2(QFile::remove("./LoggersTest.log"), "could not delete log file");

    FileLogger<> l_name3(stdout);
    QVERIFY2(l_name3.isReady(err_msg), "File logger is not ready with stdout.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with stdout.");

    FileLogger<> l_name4(stderr);
    QVERIFY2(l_name4.isReady(err_msg), "File logger is not ready with stderr.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with stderr.");
}

void tst_QLogSystem::minLogLvl()
{

    FileLogger<LvlLogPrefix>* fileLogger = new FileLogger<LvlLogPrefix>("./LoggersTest.log");
    char forbidden[] = "should not be shown!";

    QString err_msg;
    QVERIFY2(fileLogger->isReady(err_msg), "File logger is not ready with local dir.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with local dir");

    fileLogger->setMinLogLvl(LlWarning);

    LogSystem::getInstance().addLogger(fileLogger);
    LogSystem::getInstance().log(LlInfo, forbidden);
    LogSystem::getInstance().clear();

    QFile file("./LoggersTest.log");
    QVERIFY2(file.open(QFile::ReadOnly), "Could not open log file for reading");
    QByteArray dataBuf= file.readLine();
    file.close();

    QVERIFY2(dataBuf.isEmpty(), "The logger has written record with invalid Log Level");
}

void tst_QLogSystem::loggerOutput_data()
{
    QTest::addColumn<LogLevel>("lvl");
    QTest::addColumn<QString>("fstr");
    QTest::addColumn<int>("intIn");
    QTest::addColumn<QString>("chIn");
    QTest::addColumn<double>("dlIn");
    QTest::addColumn<QString>("expected");

    QTest::newRow("test 'log' method dbg level")<<LlDbg<<"Test: '%d' '%s' '%.2f'"<<12<<"string"<<12.25<<"[Debug] Test: '12' 'string' '12.25'\n";
    QTest::newRow("test 'log' method info level")<<LlInfo<<"Test: '%d' '%s' '%.2f'"<<12<<"string"<<12.25<<"[Info] Test: '12' 'string' '12.25'\n";
    QTest::newRow("test 'log' method warn level")<<LlWarning<< "Test: '%d' '%s' '%.2f'"<<12<<"string"<<12.25<<"[Warning] Test: '12' 'string' '12.25'\n";
    QTest::newRow("test 'log' method err level")<<LlError<< "Test: '%d' '%s' '%.2f'"<<12<<"string"<<12.25<<"[Error] Test: '12' 'string' '12.25'\n";
    QTest::newRow("test 'log' method err level")<<LlCritical<< "Test: '%d' '%s' '%.2f'"<<12<<"string"<<12.25<<"[Critical] Test: '12' 'string' '12.25'\n";
}


void tst_QLogSystem::loggerOutput()
{
    QFETCH(LogLevel, lvl);
    QFETCH(QString, fstr);
    QFETCH(int, intIn);
    QFETCH(QString, chIn);
    QFETCH(double, dlIn);
    QFETCH(QString, expected);

    FileLogger<LvlLogPrefix>* fileLogger = new FileLogger<LvlLogPrefix>("./LoggersTest.log");
    fileLogger->setMinLogLvl(LlDbg);

    QString err_msg;
    QVERIFY2(fileLogger->isReady(err_msg), "File logger is not ready with local dir.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with local dir");

    LogSystem::getInstance().addLogger(fileLogger);
    LogSystem::getInstance().log(lvl, qPrintable(fstr), intIn,  qPrintable(chIn), dlIn);
    LogSystem::getInstance().clear();

    QFile file("./LoggersTest.log");
    QVERIFY2(file.open(QFile::ReadOnly), "Could not open log file for reading");
    QByteArray dataBuf= file.readLine();

    QCOMPARE( QString(dataBuf.data()), expected);

    file.close();
}

void tst_QLogSystem::qDebugOutput_data()
{
    QTest::addColumn<QString>("fstr");
    QTest::addColumn<int>("intIn");
    QTest::addColumn<QString>("chIn");
    QTest::addColumn<double>("dlIn");
    QTest::addColumn<QString>("expected");

    QTest::newRow("test 'log' method dbg level")<<"Test: '%d' '%s' '%.2f'"<<12<<"string"<<12.25<<"[Debug] Test: '12' 'string' '12.25'\n";
}


void tst_QLogSystem::qDebugOutput()
{
    QFETCH(QString, fstr);
    QFETCH(int, intIn);
    QFETCH(QString, chIn);
    QFETCH(double, dlIn);
    QFETCH(QString, expected);

    FileLogger<LvlLogPrefix>* fileLogger = new FileLogger<LvlLogPrefix>("./LoggersTest.log");
    fileLogger->setMinLogLvl(LlDbg);

    QString err_msg;
    QVERIFY2(fileLogger->isReady(err_msg), "File logger is not ready with local dir.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with local dir");

    LogSystem::getInstance().addLogger(fileLogger);
    qDebug(qPrintable(fstr), intIn,  qPrintable(chIn), dlIn);
    LogSystem::getInstance().clear();

    QFile file("./LoggersTest.log");
    QVERIFY2(file.open(QFile::ReadOnly), "Could not open log file for reading");
    QByteArray dataBuf= file.readLine();

    QCOMPARE( QString(dataBuf.data()), expected);

    file.close();
    QVERIFY2(QFile::remove("./LoggersTest.log"), "could not delete log file");
}

void tst_QLogSystem::qWarningOutput_data()
{
    QTest::addColumn<QString>("fstr");
    QTest::addColumn<int>("intIn");
    QTest::addColumn<QString>("chIn");
    QTest::addColumn<double>("dlIn");
    QTest::addColumn<QString>("expected");

    QTest::newRow("test 'log' method warn level")<< "Test: '%d' '%s' '%.2f'"<<12<<"string"<<12.25<<"[Warning] Test: '12' 'string' '12.25'\n";
}

void tst_QLogSystem::qWarningOutput()
{
    QFETCH(QString, fstr);
    QFETCH(int, intIn);
    QFETCH(QString, chIn);
    QFETCH(double, dlIn);
    QFETCH(QString, expected);

    FileLogger<LvlLogPrefix>* fileLogger = new FileLogger<LvlLogPrefix>("./LoggersTest.log");
    fileLogger->setMinLogLvl(LlDbg);

    QString err_msg;
    QVERIFY2(fileLogger->isReady(err_msg), "File logger is not ready with local dir.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with local dir");

    LogSystem::getInstance().addLogger(fileLogger);
    qWarning(qPrintable(fstr), intIn,  qPrintable(chIn), dlIn);
    LogSystem::getInstance().clear();

    QFile file("./LoggersTest.log");
    QVERIFY2(file.open(QFile::ReadOnly), "Could not open log file for reading");
    QByteArray dataBuf= file.readLine();

    QCOMPARE(QString(dataBuf.data()), expected);

    file.close();
}

void tst_QLogSystem::sysLoggerOutput()
{
#if defined(Q_OS_UNIX)
    QString expected = "[Debug] Test: '12' 'string' '12.25'\n";

    SysLogger<LvlLogPrefix>* sysLogger = new SysLogger<LvlLogPrefix>("QMail logger test", LOG_PID, LOG_LOCAL7);
    sysLogger->setMinLogLvl(LlDbg);

    QString err_msg;
    QVERIFY2(sysLogger->isReady(err_msg), "File logger is not ready with local dir.");
    QVERIFY2(err_msg.isEmpty(), "File logger has obtained error message with local dir");

    LogSystem::getInstance().addLogger(sysLogger);
    LogSystem::getInstance().log(LlDbg, "Test: '%d' '%s' '%.2f'", 12,  "string", 12.25);
    LogSystem::getInstance().clear();

    QWARN (qPrintable(tr("Now You should see the string") + expected + tr("at syslog output LOG_LOCAL7, LOG_INFO")));
#endif // defined(Q_OS_UNIX)
}
