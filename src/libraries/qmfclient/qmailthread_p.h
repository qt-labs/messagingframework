#ifndef QMAILTHREAD_P_H
#define QMAILTHREAD_P_H

#include "qmailthread.h"


class QMailThreadPrivate : public QPrivateImplementationBase
{
public:
    QMailThreadPrivate()
        : QPrivateImplementationBase(this),
          messageCount(0),
          unreadCount(0),
          status(0)
    {
    }

    QMailThreadId id;
    uint messageCount;
    uint unreadCount;
    QString serverUid;
    QMailAccountId parentAccountId;
    QString subject;
    QString preview;
    QString senders;
    QMailTimeStamp lastDate;
    QMailTimeStamp startedDate;
    quint64 status;
};


#endif // QMAILTHREAD_P_H
