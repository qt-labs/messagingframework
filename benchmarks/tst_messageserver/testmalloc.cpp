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

#include "testmalloc.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QByteArray>

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
        return D->peak_usable.load();
    else
        return -1;
}

int TestMalloc::peakTotal()
{
    if (!D) TestMallocPrivate::init();
    if (D->valid)
        return D->peak_total.load();
    else
        return -1;
}

int TestMalloc::nowUsable()
{
    if (!D) TestMallocPrivate::init();
    if (D->valid)
        return D->now_usable.load();
    else
        return -1;
}

int TestMalloc::nowTotal()
{
    if (!D) TestMallocPrivate::init();
    if (D->valid)
        return D->now_usable.load() + D->now_overhead.load();
    else
        return -1;
}

void TestMalloc::resetPeak()
{
    if (!D) TestMallocPrivate::init();
    D->peak_usable.fetchAndStoreOrdered(D->now_usable.load());
    D->peak_total.fetchAndStoreOrdered(D->now_usable.load() + D->now_overhead.load());
}

void TestMalloc::resetNow()
{
    if (!D) TestMallocPrivate::init();
    D->now_usable.store(0);
    D->now_overhead.store(0);
}

#ifndef __MALLOC_HOOK_VOLATILE
#define __MALLOC_HOOK_VOLATILE
#endif

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook) (void) = TestMallocPrivate::init;

void TestMallocPrivate::init()
{
    /*
        When using glibc malloc, this function will be called before any heap allocation.
        When using other malloc and when running under valgrind, we might get called after
        some heap allocation.
    */
    struct mallinfo info = mallinfo();
    static TestMallocPrivate testmalloc;
    testmalloc.now_usable.store(info.uordblks);
    testmalloc.now_overhead.store(0); /* cannot get this figure, but should be close to 0. */
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
    int before = this->now_usable.load();
    int during;
    int after;
    char* array = 0;
    {
        QByteArray ba;
        ba.resize(512);
        array = new char[512];

        during = this->now_usable.load();
    }
    delete [] array;
    after = this->now_usable.load();

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
    if (now_usable.load() > peak_usable.load()) {
        peak_usable.fetchAndStoreOrdered(now_usable.load());
    }
    if (now_usable.load() + now_overhead.load() > peak_total.load()) {
        peak_total.fetchAndStoreOrdered(now_usable.load() + now_overhead.load());
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

