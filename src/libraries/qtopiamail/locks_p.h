/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef SEMAPHORE_P_H
#define SEMAPHORE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// We need slightly different semantics to those of QSystemMutex - all users of
// the qtopiamail library are peers, so no single caller is the owner.  We will
// allow the first library user to create the semaphore, and any subsequent users
// will attach to the same semaphore set.  No-one will close the semaphore set,
// we will rely on process undo to maintain sensible semaphore values as
// clients come and go...

class QString;

class ProcessMutexPrivate;

class ProcessMutex
{
public:
    ProcessMutex(const QString &path, int id = 0);
    ~ProcessMutex();

    bool lock(int milliSec);
    void unlock();

private:
    ProcessMutex(const ProcessMutex &);
    const ProcessMutex& operator=(const ProcessMutex &);

    ProcessMutexPrivate* d;
};

class ProcessReadLockPrivate;

class ProcessReadLock
{
public:
    ProcessReadLock(const QString &path, int id = 0);
    ~ProcessReadLock();

    void lock();
    void unlock();

    bool wait(int milliSec);

private:
    ProcessReadLock(const ProcessReadLock &);
    const ProcessReadLock& operator=(const ProcessReadLock &);

    ProcessReadLockPrivate* d;
};

#endif
