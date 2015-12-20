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

    char newmod[] = "NewModule";
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
