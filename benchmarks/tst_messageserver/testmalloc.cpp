/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "testmalloc.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

/*
    Define this to have testmalloc do more extensive selftests to help verify
    that all allocations are going through our overridden allocator.
    This has a negative impact on performance.
*/
//#define TEST_TESTMALLOC

/* Additional amount of memory allocated by malloc for each chunk */
#define CHUNK_OVERHEAD (2*sizeof(size_t))

struct TestMallocPrivate
{
    static void init();
    void selftest();

    TestMallocPrivate()
        : valid(false)
        , peak_usable(0)
        , peak_total(0)
        , now_usable(0)
        , now_overhead(0)
    {
        instance = this;
    }

    ~TestMallocPrivate()
    {
        instance = 0;
    }

    void updatePeak();

    static TestMallocPrivate* instance;

#ifdef TEST_TESTMALLOC
    QAtomicInt inTestMalloc;
    static void afterMorecore();
#endif

    bool        valid;
    QAtomicInt  peak_usable;
    QAtomicInt  peak_total;
    QAtomicInt  now_usable;
    QAtomicInt  now_overhead;
};

#define D (TestMallocPrivate::instance)

TestMallocPrivate* TestMallocPrivate::instance = 0;

/*
    libc versions of functions.  These are aliases for the libc malloc which we can
    use to avoid using dlsym to look up malloc.  That is troublesome because dlsym
    itself will call malloc.
*/
extern "C" void* __libc_malloc(size_t);
extern "C" void* __libc_calloc(size_t,size_t);
extern "C" void* __libc_realloc(void*,size_t);
extern "C" void* __libc_memalign(size_t,size_t);
extern "C" void  __libc_free(void*);

int TestMalloc::peakUsable()
{
    if (!D) TestMallocPrivate::init();
    if (D->valid)
        return D->peak_usable;
    else
        return -1;
}

int TestMalloc::peakTotal()
{
    if (!D) TestMallocPrivate::init();
    if (D->valid)
        return D->peak_total;
    else
        return -1;
}

int TestMalloc::nowUsable()
{
    if (!D) TestMallocPrivate::init();
    if (D->valid)
        return D->now_usable;
    else
        return -1;
}

int TestMalloc::nowTotal()
{
    if (!D) TestMallocPrivate::init();
    if (D->valid)
        return D->now_usable + D->now_overhead;
    else
        return -1;
}

void TestMalloc::resetPeak()
{
    if (!D) TestMallocPrivate::init();
    D->peak_usable.fetchAndStoreOrdered(D->now_usable);
    D->peak_total.fetchAndStoreOrdered(D->now_usable + D->now_overhead);
}

void TestMalloc::resetNow()
{
    if (!D) TestMallocPrivate::init();
    D->now_usable = 0;
    D->now_overhead = 0;
}

void (*__malloc_initialize_hook) (void) = TestMallocPrivate::init;

void TestMallocPrivate::init()
{
    /*
        When using glibc malloc, this function will be called before any heap allocation.
        When using other malloc and when running under valgrind, we might get called after
        some heap allocation.
    */
    struct mallinfo info = mallinfo();
    static TestMallocPrivate testmalloc;
    testmalloc.now_usable = info.uordblks;
    testmalloc.now_overhead = 0; /* cannot get this figure, but should be close to 0. */
    TestMalloc::resetPeak();
    testmalloc.selftest();

    /* Turn off mmap so that all blocks have a fixed overhead. */
    mallopt(M_MMAP_MAX, 0);

#ifdef TEST_TESTMALLOC
    __after_morecore_hook = &TestMallocPrivate::afterMorecore;
    mallopt(M_TRIM_THRESHOLD, 0);
    mallopt(M_TOP_PAD, 0);
#endif
}

#ifdef TEST_TESTMALLOC
void TestMallocPrivate::afterMorecore()
{
    TestMallocPrivate* d = TestMallocPrivate::instance;
    if (d && (0 == (int)(d->inTestMalloc))) {
        fprintf(stderr, "Some memory allocation failed to go through hooks!\n");
        fflush(stderr);
        abort();
    }
}
#endif

void TestMallocPrivate::selftest()
{
    int before = this->now_usable;
    int during;
    int after;
    char* array = 0;
    {
        QByteArray ba;
        ba.resize(512);
        array = new char[512];

        during = this->now_usable;
    }
    delete [] array;
    after = this->now_usable;

    if (!(during >= before+1024)) {
        qWarning("Heap usage measurement fail: heap before byte array was %d, during was %d (expected at least %d).  Heap usage will not be measured.", before, during, before + 1024);
        return;
    }

    /*
        After and before ideally would be the same, but in practice memory may have been allocated
        or freed, e.g. by the dynamic linker looking up QByteArray symbols.
    */
    if (qAbs(after - before) >= 128) {
        qWarning("Heap usage measurement fail: heap before byte array was %d, after was %d (expected after to be approximately equal to before).  Heap usage will not be measured.", before, after);
        return;
    }

    valid = true;
}

void TestMallocPrivate::updatePeak()
{
    if (now_usable > peak_usable) {
        peak_usable.fetchAndStoreOrdered(now_usable);
    }
    if (now_usable + now_overhead > peak_total) {
        peak_total.fetchAndStoreOrdered(now_usable + now_overhead);
    }
}

#ifdef TEST_TESTMALLOC
#define REF                     \
    bool didref = false;        \
    do { if (D) {               \
        didref = true;          \
        D->inTestMalloc.ref();  \
    } } while(0)
#define DEREF if (didref) D->inTestMalloc.deref()
#else
#define REF     do {} while(0)
#define DEREF   do {} while(0)
#endif

extern "C" void* malloc(size_t size)
{
    REF;
    void* out = __libc_malloc(size);
    DEREF;
    if (out && D) {
        D->now_usable.fetchAndAddOrdered(malloc_usable_size(out));
        D->now_overhead.fetchAndAddOrdered(CHUNK_OVERHEAD);
        D->updatePeak();
    }
    return out;
}

extern "C" void* calloc(size_t nmemb, size_t size)
{
    REF;
    void* out = __libc_calloc(nmemb, size);
    DEREF;
    if (out && D) {
        D->now_usable.fetchAndAddOrdered(malloc_usable_size(out));
        D->now_overhead.fetchAndAddOrdered(CHUNK_OVERHEAD);
        D->updatePeak();
    }
    return out;
}

extern "C" void* realloc(void* in, size_t size)
{
    size_t oldsize = (D && in) ? malloc_usable_size(in) : 0;

    REF;
    void* out = __libc_realloc(in,size);
    DEREF;

    if (D) {
        D->now_usable.fetchAndAddOrdered(malloc_usable_size(out) - oldsize);
        /* Overhead is affected only if old size was 0 */
        if (!oldsize) D->now_overhead.fetchAndAddOrdered(CHUNK_OVERHEAD);
        D->updatePeak();
    }
    return out;
}

extern "C" void* memalign(size_t alignment, size_t size)
{
    REF;
    void* out = __libc_memalign(alignment, size);
    DEREF;
    if (out && D) {
        D->now_usable.fetchAndAddOrdered(malloc_usable_size(out));
        D->now_overhead.fetchAndAddOrdered(CHUNK_OVERHEAD);
        D->updatePeak();
    }
    return out;
}

extern "C" void free(void* in)
{
    size_t oldsize = (D && in) ? malloc_usable_size(in) : 0;
    REF;
    __libc_free(in);
    if (D && oldsize) {
        D->now_usable.fetchAndAddOrdered(-oldsize);
        D->now_overhead.fetchAndAddOrdered(-CHUNK_OVERHEAD);
        D->updatePeak();
    }
    DEREF;
}

/* Force new/delete to go through malloc so we can profile them too. */
void* operator new[](size_t size)
{
    return ::malloc(size);
}

void* operator new(size_t size)
{
    return ::malloc(size);
}

void operator delete[](void* p)
{
    ::free(p);
}

void operator delete[](void* p, size_t /*size*/)
{
    ::free(p);
}

void operator delete(void* p)
{
    ::free(p);
}

void operator delete(void* p, size_t /*size*/)
{
    ::free(p);
}

