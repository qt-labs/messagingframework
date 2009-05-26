/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGESET_P_H
#define QMAILMESSAGESET_P_H

#include "qmailmessageset.h"


// These classes are implemented via qmailmessageset.cpp and qmailinstantiations.cpp

class QMailMessageSetContainerPrivate : public QPrivateNoncopyableBase
{
public:
    template<typename Subclass>
    QMailMessageSetContainerPrivate(Subclass *p, QMailMessageSetContainer *parent)
        : QPrivateNoncopyableBase(p),
          _container(parent)
    {
    }

    QMailMessageSetContainer *_container;
    QList<QMailMessageSet*> _children;
};

#endif

