/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QSCOPEDCONNECTION_H
#define QSCOPEDCONNECTION_H

#include <QObject>
#include <QPointer>

class QScopedConnection
{
public:
    QScopedConnection(QObject*, char const*, QObject*, char const*);
    ~QScopedConnection();
private:
    QPointer<QObject> m_sender;
    QByteArray        m_signal;
    QPointer<QObject> m_receiver;
    QByteArray        m_slot;
};

#endif

