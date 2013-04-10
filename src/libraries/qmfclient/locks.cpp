/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "locks_p.h"
#include "qmaillog.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>


#if !defined(Q_OS_MAC)
union semun {
    int val;
};
#endif

namespace {

int pathIdentifier(const QString &filePath, int id)
{
    return static_cast<int>(::ftok(filePath.toLatin1(), id));
}

class Semaphore
{
    int m_id;
    bool m_remove;
    int m_semId;
    int m_initialValue;

    void operation(struct sembuf *op);

public:
    Semaphore(int id, bool remove, int initial);
    ~Semaphore();

    void decrement();
    void increment();

    void waitForZero();
};

Semaphore::Semaphore(int id, bool remove, int initial)
    : m_id(id),
      m_remove(false),
      m_semId(-1),
      m_initialValue(initial)
{
    m_semId = ::semget(m_id, 1, 0);

    if (m_semId == -1) {
        if (errno == ENOENT) {
            // This lock does not exist
            m_semId = ::semget(m_id, 1, IPC_CREAT | IPC_EXCL | S_IRWXU);
            if (m_semId == -1) {
                if (errno == EEXIST) {
                    // Someone else won the race to create
                    m_semId = ::semget(m_id, 1, 0);
                }

                if (m_semId == -1) {
                    qMailLog(Messaging) << "Semaphore: Unable to create semaphore ID:" << m_id << ":" << ::strerror(errno);
                }
            } else {
                // We created the semaphore
                m_remove = remove;

                union semun arg;
                arg.val = m_initialValue;
                int status = ::semctl(m_semId, 0, SETVAL, arg);
                if (status == -1) {
                    m_semId = -1;
                    qMailLog(Messaging) << "Semaphore: Unable to initialize semaphore ID:" << m_id << ":" << ::strerror(errno);
                }
            }
        } else {
            qMailLog(Messaging) << "Semaphore: Unable to get semaphore ID:" << m_id << ":" << ::strerror(errno);
        }
    }
}

Semaphore::~Semaphore()
{
    if (m_remove) {
        int status = ::semctl(m_semId, 0, GETVAL);
        if (status == -1) {
            qMailLog(Messaging) << "Semaphore: Unable to get value of semaphore ID:" << m_id << ":" << ::strerror(errno);
        } else { 
            if (status == m_initialValue) {
                // No other holder of this semaphore
                status = ::semctl(m_semId, 0, IPC_RMID);
                if (status == -1) {
                    qMailLog(Messaging) << "Semaphore: Unable to destroy semaphore ID:" << m_id << ":" << ::strerror(errno);
                }
            } else {
                qMailLog(Messaging) << "Semaphore: semaphore ID:" << m_id << "still active:" << status;
            }
        }
    }
}

void Semaphore::decrement()
{
    if (m_semId != -1) {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = -1;
        op.sem_flg = SEM_UNDO;

        operation(&op);
    } else {
        qMailLog(Messaging) << "Semaphore: Unable to decrement invalid semaphore ID:" << m_id;
    }
}

void Semaphore::increment()
{
    if (m_semId != -1) {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = 1;
        op.sem_flg = SEM_UNDO;

        operation(&op);
    } else {
        qMailLog(Messaging) << "Semaphore: Unable to increment invalid semaphore ID:" << m_id;
    }
}

void Semaphore::waitForZero()
{
    if (m_semId != -1) {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = 0;
        op.sem_flg = 0;

        operation(&op);
    } else {
        qMailLog(Messaging) << "Semaphore: Unable to wait for zero on invalid semaphore ID:" << m_id;
    }
}

void Semaphore::operation(struct sembuf *op)
{
    if(::semop(m_semId, op, 1) != 0) {
        qMailLog(Messaging) << "Semaphore operation had an error" << errno;
    }

}

}


class ProcessMutexPrivate : private Semaphore
{
public:
    ProcessMutexPrivate(int id) : Semaphore(id, false, 1) {}

    void lock() { decrement(); }
    void unlock() { increment(); }
};

ProcessMutex::ProcessMutex(const QString &path, int id)
    : d(new ProcessMutexPrivate(pathIdentifier(path, id)))
{
}

ProcessMutex::~ProcessMutex()
{
    delete d;
}

void ProcessMutex::lock()
{
    d->lock();
}

void ProcessMutex::unlock()
{
    d->unlock();
}


class ProcessReadLockPrivate : private Semaphore
{
public:
    ProcessReadLockPrivate(int id) : Semaphore(id, false, 0) {}

    void lock() { increment(); }
    void unlock() { decrement(); }

    void wait() { waitForZero(); }
};

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

void ProcessReadLock::wait()
{
    d->wait();
}
