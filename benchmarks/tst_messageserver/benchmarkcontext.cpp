/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "benchmarkcontext.h"
#include "testfsusage.h"
#include "testmalloc.h"
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

#include <QDebug>
#include <QDir>
#include <QTest>

#include <unistd.h>

#undef HAVE_TICK_COUNTER // not useful for this test

class BenchmarkContextPrivate
{
public:
    bool xml;
    qint64 qmfUsage;
    QTime time;
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

#ifndef Q_OS_SYMBIAN
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
            QString match = QString("callgrind.out.%1.").arg(::getpid());
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
#ifndef Q_OS_SYMBIAN
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

