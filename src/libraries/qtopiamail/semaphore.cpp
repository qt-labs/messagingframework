/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "semaphore_p.h"
#include "qmaillog.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>


#if !defined(Q_OS_SYMBIAN)
union semun {
    int val;
};
#endif

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

bool Semaphore::decrement(int milliSec)
{
    if (m_semId != -1) {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = -1;
        op.sem_flg = SEM_UNDO;

        return operation(&op, milliSec);
    } else {
        qMailLog(Messaging) << "Semaphore: Unable to decrement invalid semaphore ID:" << m_id;
    }

    return false;
}

bool Semaphore::increment(int milliSec)
{
    if (m_semId != -1) {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = 1;
        op.sem_flg = SEM_UNDO;

        return operation(&op, milliSec);
    } else {
        qMailLog(Messaging) << "Semaphore: Unable to increment invalid semaphore ID:" << m_id;
    }

    return false;
}

bool Semaphore::waitForZero(int milliSec)
{
    if (m_semId != -1) {
        struct sembuf op;
        op.sem_num = 0;
        op.sem_op = 0;
        op.sem_flg = 0;

        return operation(&op, milliSec);
    } else {
        qMailLog(Messaging) << "Semaphore: Unable to wait for zero on invalid semaphore ID:" << m_id;
    }

    return false;
}

bool Semaphore::operation(struct sembuf *op, int milliSec)
{
#ifdef QTOPIA_HAVE_SEMTIMEDOP
    if (milliSec >= 0) {
        struct timespec ts;
        ts.tv_sec = milliSec / 1000;
        ts.tv_nsec = (milliSec % 1000) * 1000000;
        return (::semtimedop(m_semId, op, 1, &ts) != -1);
    }
#else
    Q_UNUSED(milliSec);
#endif

    return (::semop(m_semId, op, 1) != -1);
}

