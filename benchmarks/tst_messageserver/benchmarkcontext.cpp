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
#include "3rdparty/cycle_p.h"
#include <valgrind/valgrind.h>

#include <qmailnamespace.h>

#include <QDebug>
#include <QDir>
#include <QTest>

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

    TestMalloc::resetNow();
    TestMalloc::resetPeak();
}

BenchmarkContext::~BenchmarkContext()
{
    if (!QTest::currentTestFailed()) {
        qint64 newQmfUsage = TestFsUsage::usage(QMail::dataPath());
#ifdef HAVE_TICK_COUNTER
        CycleCounterTicks newTicks = getticks();
#endif

        int heapUsageTotal  = TestMalloc::peakTotal()/1024;
        int heapUsageUsable = TestMalloc::peakUsable()/1024;
        int ms = d->time.elapsed();
        quint64 cycles = quint64(elapsed(newTicks,d->ticks));
        qint64 diskUsage = (newQmfUsage - d->qmfUsage) / 1024;
        if (d->xml) {
            if (!RUNNING_ON_VALGRIND) {
                fprintf(stdout, "<BenchmarkResult metric=\"heap_usage\" tag=\"%s_\" value=\"%d\" iterations=\"1\"/>\n", QTest::currentDataTag(), heapUsageTotal);
            }
            fprintf(stdout, "<BenchmarkResult metric=\"disk_usage\" tag=\"%s_\" value=\"%lld\" iterations=\"1\"/>\n", QTest::currentDataTag(), diskUsage);
            fprintf(stdout, "<BenchmarkResult metric=\"cycles\" tag=\"%s_\" value=\"%llu\" iterations=\"1\"/>\n", QTest::currentDataTag(), cycles);
            fprintf(stdout, "<BenchmarkResult metric=\"walltime\" tag=\"%s_\" value=\"%d\" iterations=\"1\"/>\n", QTest::currentDataTag(), ms);
            fflush(stdout);
        }
        else {
            if (!RUNNING_ON_VALGRIND) {
                qWarning() << "Peak heap usage (kB):" << heapUsageTotal << "total (" << heapUsageUsable << "usable )";
            }
            qWarning() << "Change in homedir disk usage:" << diskUsage << "kB";
            qWarning("Cycles: %llu", cycles);
            qWarning() << "Execution time:" << ms << "ms";
        }
    }

    delete d;
    d = 0;
}

