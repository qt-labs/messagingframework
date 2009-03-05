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

struct sembuf;

// We need slightly different semantics to those of QSystemMutex - all users of
// the qtopiamail library are peers, so no single caller is the owner.  We will
// allow the first library user to create the semaphore, and any subsequent users
// will attach to the same semaphore set.  No-one will close the semaphore set,
// we will rely on process undo to maintain sensible semaphore values as
// clients come and go...

class Semaphore
{
    int m_id;
    bool m_remove;
    int m_semId;
    int m_initialValue;

    bool operation(struct sembuf *op, int milliSec);

public:
    Semaphore(int id, bool remove, int initial);
    ~Semaphore();

    bool decrement(int milliSec = -1);
    bool increment(int milliSec = -1);

    bool waitForZero(int milliSec = -1);
};

class ProcessMutex : public Semaphore
{
public:
    ProcessMutex(int id) : Semaphore(id, false, 1) {}

    bool lock(int milliSec) { return decrement(milliSec); }
    void unlock() { increment(); }
};

class ProcessReadLock : public Semaphore
{
public:
    ProcessReadLock(int id) : Semaphore(id, false, 0) {}

    void lock() { increment(); }
    void unlock() { decrement(); }

    bool wait(int milliSec) { return waitForZero(milliSec); }
};

#endif
