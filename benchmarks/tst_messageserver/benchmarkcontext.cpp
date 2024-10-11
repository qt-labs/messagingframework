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

#include "benchmarkcontext.h"
#include "testfsusage.h"
#if defined(Q_OS_LINUX)
#include "testmalloc.h"
#endif
#ifdef HAVE_VALGRIND
#include "3rdparty/cycle_p.h"
#include "3rdparty/callgrind_p.h"
#include "3rdparty/valgrind_p.h"
#else
#define RUNNING_ON_VALGRIND 0
#define CALLGRIND_ZERO_STATS
#define CALLGRIND_DUMP_STATS
#endif

#include <qmailnamespace.h>

#include <QCoreApplication>
#include <qmaillog.h>
#include <QDir>
#include <QElapsedTimer>
#include <QTest>

#undef HAVE_TICK_COUNTER // not useful for this test

class BenchmarkContextPrivate
{
public:
    bool xml;
    qint64 qmfUsage;
    QElapsedTimer time;
#ifdef HAVE_TICK_COUNTER
    CycleCounterTicks ticks;
#endif
};

BenchmarkContext::BenchmarkContext(bool xml)
    : d(new BenchmarkContextPrivate)
{
    d->xml = xml;
    d->qmfUsage = TestFsUsage::usage(QMail::dataPath());

#ifdef HAVE_TICK_COUNTER
    d->ticks = getticks();
#endif

    d->time.start();

#if defined(Q_OS_LINUX)
    TestMalloc::resetNow();
    TestMalloc::resetPeak();
#endif
    CALLGRIND_ZERO_STATS;
}

BenchmarkContext::~BenchmarkContext()
{
    if (!QTest::currentTestFailed()) {
        CALLGRIND_DUMP_STATS;

        if (RUNNING_ON_VALGRIND) {
            // look for callgrind.out.<PID>.<NUM> in CWD
            QDir dir;
            QFile file;
            QString match = QString("callgrind.out.%1.").arg(QCoreApplication::applicationPid());
            foreach (QString const& entry, dir.entryList(QDir::Files,QDir::Name)) {
                if (entry.startsWith(match)) {
                    file.setFileName(entry);
                }
            }

            if (file.fileName().isEmpty()) {
                qWarning("I'm running on valgrind, but couldn't find callgrind.out file...");
                return;
            }

            if (!file.open(QIODevice::ReadOnly)) {
                qWarning("Failed to open %s: %s", qPrintable(file.fileName()),
                    qPrintable(file.errorString()));
            }

            QList<QByteArray> lines = file.readAll().split('\n');
            qint64 ir = -1;
            bool is_ir = false;
            foreach (QByteArray const& line, lines) {
                if (line == "events: Ir") {
                    is_ir = true;
                    continue;
                }
                if (is_ir) {
                    if (line.startsWith("summary: ")) {
                        bool ok;
                        ir = line.mid(sizeof("summary: ")-1).toLongLong(&ok);
                        if (!ok) ir = -1;
                    }
                    is_ir = false;
                    break;
                }
            }
            if (ir == -1) {
                qWarning("Found callgrind.out file, but it doesn't seem to contain a valid "
                    "Ir count...");
                return;
            }

            if (d->xml) {
                fprintf(stdout, "<BenchmarkResult metric=\"callgrind\" tag=\"%s\" value=\"%lld\" iterations=\"1\"/>\n", QTest::currentDataTag(), ir);
            }
            else {
                qWarning("callgrind instruction count: %lld", ir);
            }
            // callgrind invalidates rest of results
            return;
        }

#ifdef HAVE_TICK_COUNTER
        CycleCounterTicks newTicks = getticks();
#endif

        // Note, kilo means 1000, not 1024 !
#if defined(Q_OS_LINUX)
        int heapUsageTotal  = TestMalloc::peakTotal()/1000;
        int heapUsageUsable = TestMalloc::peakUsable()/1000;
#else
        int heapUsageTotal  = 0;
        int heapUsageUsable = 0;
#endif
        int ms = d->time.elapsed();
        qint64 newQmfUsage = TestFsUsage::usage(QMail::dataPath());
#ifdef HAVE_TICK_COUNTER
        quint64 cycles = quint64(elapsed(newTicks,d->ticks));
#endif
        qint64 diskUsage = (newQmfUsage - d->qmfUsage) / 1000;
        if (d->xml) {
            if (!RUNNING_ON_VALGRIND) {
                fprintf(stdout, "<BenchmarkResult metric=\"kilobytes heap usage\" tag=\"%s\" value=\"%d\" iterations=\"1\"/>\n", QTest::currentDataTag(), heapUsageTotal);
            }
            fprintf(stdout, "<BenchmarkResult metric=\"kilobytes disk usage\" tag=\"%s\" value=\"%lld\" iterations=\"1\"/>\n", QTest::currentDataTag(), diskUsage);
#ifdef HAVE_TICK_COUNTER
            fprintf(stdout, "<BenchmarkResult metric=\"cycles\" tag=\"%s\" value=\"%llu\" iterations=\"1\"/>\n", QTest::currentDataTag(), cycles);
#endif
            // `milliseconds walltime' would be better, but keep `walltime' for benchlib
            // compatibility
            fprintf(stdout, "<BenchmarkResult metric=\"walltime\" tag=\"%s\" value=\"%d\" iterations=\"1\"/>\n", QTest::currentDataTag(), ms);
            fflush(stdout);
        }
        else {
            if (!RUNNING_ON_VALGRIND) {
                qWarning() << "Peak heap usage (kB):" << heapUsageTotal << "total (" << heapUsageUsable << "usable )";
            }
            qWarning() << "Change in homedir disk usage:" << diskUsage << "kB";
#ifdef HAVE_TICK_COUNTER
            qWarning("Cycles: %llu", cycles);
#endif
            qWarning() << "Execution time:" << ms << "ms";
        }
    }

    delete d;
    d = 0;
}

