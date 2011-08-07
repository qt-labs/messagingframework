/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QTest>
#include <qmaillog.h>
#include "qloggers.h"
#include <QSettings>

class tst_QMailLog : public QObject
{
    Q_OBJECT

public:
    tst_QMailLog() {}
    virtual ~tst_QMailLog() {}

private slots:
    void test_qmaillog();
    void test_logsystem();
    void test_loggers();
};

QTEST_MAIN(tst_QMailLog)

#include "tst_qmaillog.moc"

void tst_QMailLog::test_qmaillog()
{
    // enable stderr logging and off syslog
    QSettings settings("nokia", "qmf");
    settings.setValue("SysLog/Enabled", false);
    settings.setValue("StdStreamLog/StdErrEnabled", true);

    qMailLoggersRecreate(settings.organizationName(), settings.applicationName(), 0);

    char const* newmod = "NewModule";
    qmf_registerLoggingFlag(newmod); // no way to verify if it is registered

    QDebug dbg = QLogBase::log(newmod);
    // no way to verify if the category existed or not - check output messages??
}

void tst_QMailLog::test_logsystem()
{
    // for code coverage only

    qWarning() << "warning message";
    qCritical() << "critical message";
    qDebug() << "debug message";

    // fatal message will exit so commented
    //qFatal() << "fatal message";

}

class TestBaseLoggerFoundation: public BaseLoggerFoundation
{
public:
    TestBaseLoggerFoundation(): BaseLoggerFoundation(LlInfo) {}

    void set_ready(bool ready)
    {
        if(ready)
            setReady();
        else
            setUnReady("TestBaseLoggerFoundation");
    };

    void log(LogLevel lvl, const char *fmt, va_list args)
    {
        Q_UNUSED(lvl); Q_UNUSED(fmt); Q_UNUSED(args);
    }
};

void tst_QMailLog::test_loggers()
{
    FileLogger<LvlTimeLogPrefix> logger1(stdout);
    QVERIFY(logger1.isReady());

    QCOMPARE(logger1.getMinLogLvl(), LlInfo);
    logger1.setMinLogLvl(LlDbg);
    QCOMPARE(logger1.getMinLogLvl(), LlDbg);

    // give invalid filename - so logger will not be ready
    FileLogger<LvlTimeLogPrefix> logger2("/");
    QVERIFY(!logger2.isReady());

    TestBaseLoggerFoundation logger3;
    QCOMPARE(logger3.getMinLogLvl(), LlInfo);
    logger3.set_ready(true);
    QVERIFY(logger3.isReady());
    logger3.set_ready(false);
    QVERIFY(!logger3.isReady());

}
