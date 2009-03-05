/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qscopedconnection.h"

QScopedConnection::QScopedConnection(
    QObject* sender, char const* signal,
    QObject* receiver, char const* slot)
    : m_sender(sender)
    , m_signal(signal)
    , m_receiver(receiver)
    , m_slot(slot)
{
    if (!QObject::connect(sender, signal, receiver, slot))
        Q_ASSERT(0);
}

QScopedConnection::~QScopedConnection()
{
    if (m_sender && m_receiver)
        QObject::disconnect(m_sender, m_signal, m_receiver, m_slot);
}

