/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "locks_p.h"
#include "qmaillog.h"
#include "qmailnamespace.h"
#include <windows.h>

// Because we don't guarantee that a server process will start before
// clients, and not restart without them, we need our locking mechanism
// to tolerate the disappearance of processes without cleaning up their
// locking state.
// Win32 semaphores do not support the SEM_UNDO concept, so we have to rely
// on mutexes, which are released by the kernel when their holder terminates
// unexpectedly.

namespace {

QString pathIdentifier(const QString &path, int id)
{
    // Object names do not need to correspond to paths that actually exist on Win32
    return QString("qtopiamail-%1-%2").arg(path).arg(id);
}

}


class ProcessMutexPrivate
{
public:
    ProcessMutexPrivate(const QString &path);
    ~ProcessMutexPrivate();

    bool lock(int milliSec);
    void unlock();

private:    
    HANDLE mutex;
    int count;
};

ProcessMutexPrivate::ProcessMutexPrivate(const QString &path)
    : mutex(NULL), count(0)
{
    mutex = ::CreateMutex(NULL, FALSE, reinterpret_cast<const wchar_t*>(path.utf16()));
    if (mutex == NULL) {
        qWarning() << "Unable to ceate/open mutex " << path << ":" << QMail::lastSystemErrorMessage();
    }
}

ProcessMutexPrivate::~ProcessMutexPrivate()
{
    if (mutex != NULL) {
        if (::CloseHandle(mutex) == FALSE) {
            qWarning() << "Unable to close handle:" << QMail::lastSystemErrorMessage();
        }
        mutex = NULL;
    }
}

bool ProcessMutexPrivate::lock(int milliSec)
{
    if (count) {
        // We already have this lock
        ++count;
    } else {
        DWORD rv = ::WaitForSingleObject(mutex, milliSec);
        if (rv == WAIT_FAILED) {
            qWarning() << "Unable to wait for mutex:" << QMail::lastSystemErrorMessage();
        } else if (rv != WAIT_TIMEOUT) {
            ++count;
        }
    }

    return (count > 0);
}

void ProcessMutexPrivate::unlock()
{
    if ((count > 0) && (--count == 0)) {
        if (::ReleaseMutex(mutex) == FALSE) {
            qWarning() << "Unable to release mutex:" << QMail::lastSystemErrorMessage();
        }
    }
}

ProcessMutex::ProcessMutex(const QString &path, int id)
    : d(new ProcessMutexPrivate(pathIdentifier(path, id)))
{
}

ProcessMutex::~ProcessMutex()
{
    delete d;
}

bool ProcessMutex::lock(int milliSec)
{
    return d->lock(milliSec);
}

void ProcessMutex::unlock()
{
    d->unlock();
}


class ProcessReadLockPrivate
{
public:
    ProcessReadLockPrivate(const QString &path);
    ~ProcessReadLockPrivate();

    void lock();
    void unlock();

    bool wait(int milliSec);

private:
    enum { MaxConcurrentReaders = 10 };

    HANDLE mutexes[MaxConcurrentReaders];
    HANDLE mutex;
    int count;
};

ProcessReadLockPrivate::ProcessReadLockPrivate(const QString &path)
    : mutex(NULL), count(0)
{
    for (int i = 0; i < MaxConcurrentReaders; ++i) {
        mutexes[i] = NULL;
    }

    for (int i = 0; i < MaxConcurrentReaders; ++i) {
        QString subPath = path + '-' + QString::number(i);

        mutexes[i] = ::CreateMutex(NULL, FALSE, reinterpret_cast<const wchar_t*>(subPath.utf16()));
        if (mutexes[i] == NULL) {
            qWarning() << "Unable to ceate/open mutex " << subPath << ":" << QMail::lastSystemErrorMessage();
            break;
        }
    }
}

ProcessReadLockPrivate::~ProcessReadLockPrivate()
{
    for (int i = 0; i < MaxConcurrentReaders; ++i) {
        if (mutexes[i] != NULL) {
            if (::CloseHandle(mutexes[i]) == FALSE) {
                qWarning() << "Unable to close handle:" << QMail::lastSystemErrorMessage();
            }
            mutexes[i] = NULL;
        }
    }
}

void ProcessReadLockPrivate::lock()
{
    if (count) {
        // We already have this lock
        ++count;
    } else {
        // Wait for any of the locks
        DWORD rv = ::WaitForMultipleObjects(MaxConcurrentReaders, mutexes, FALSE, INFINITE);
        if (rv == WAIT_FAILED) {
            qWarning() << "Unable to wait for mutex:" << QMail::lastSystemErrorMessage();
        } else {
            ++count;

            if ((rv >= WAIT_OBJECT_0) && (rv < (WAIT_OBJECT_0 + MaxConcurrentReaders))) {
                mutex = mutexes[(rv - WAIT_OBJECT_0)];
            } else if ((rv >= WAIT_ABANDONED_0) && (rv < (WAIT_ABANDONED_0 + MaxConcurrentReaders))) {
                mutex = mutexes[(rv - WAIT_ABANDONED_0)];
            } else {
                qWarning() << "Unexpected multiple wait result:" << rv;
            }
        }
    }
}

void ProcessReadLockPrivate::unlock()
{
    if ((count > 0) && (--count == 0)) {
        if (::ReleaseMutex(mutex) == FALSE) {
            qWarning() << "Unable to release mutex:" << QMail::lastSystemErrorMessage();
        }
    }
}

bool ProcessReadLockPrivate::wait(int milliSec)
{
    // Wait for all of the locks
    DWORD rv = ::WaitForMultipleObjects(MaxConcurrentReaders, mutexes, TRUE, milliSec);
    if (rv == WAIT_FAILED) {
        qWarning() << "Unable to wait for mutex:" << QMail::lastSystemErrorMessage();
    } else if (rv != WAIT_TIMEOUT) {
        // Release all locks
        for (int i = 0; i < MaxConcurrentReaders; ++i) {
            if (::ReleaseMutex(mutexes[i]) == FALSE) {
                qWarning() << "Unable to release mutex:" << QMail::lastSystemErrorMessage();
            }
        }

        return true;
    }

    return false;
}

ProcessReadLock::ProcessReadLock(const QString &path, int id)
    : d(new ProcessReadLockPrivate(pathIdentifier(path, id)))
{
}

ProcessReadLock::~ProcessReadLock()
{
    delete d;
}

void ProcessReadLock::lock()
{
    d->lock();
}

void ProcessReadLock::unlock()
{
    d->unlock();
}

bool ProcessReadLock::wait(int milliSec)
{
    return d->wait(milliSec);
}

