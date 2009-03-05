/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QCOPCHANNELMONITOR_H
#define QCOPCHANNELMONITOR_H

#include <qobject.h>

#if !defined(Q_QCOP_EXPORT)
#if defined(QT_BUILD_QCOP_LIB)
#define Q_QCOP_EXPORT Q_DECL_EXPORT
#else
#define Q_QCOP_EXPORT Q_DECL_IMPORT
#endif
#endif

class QCopChannelMonitorPrivate;
class QCopClient;

class Q_QCOP_EXPORT QCopChannelMonitor : public QObject
{
    Q_OBJECT
public:
    explicit QCopChannelMonitor(const QString& channel, QObject *parent=0);
    virtual ~QCopChannelMonitor();

    QString channel() const;

    enum State
    {
        Unknown,
        Registered,
        Unregistered
    };

    QCopChannelMonitor::State state() const;

Q_SIGNALS:
    void registered();
    void unregistered();

private:
    QCopChannelMonitorPrivate* d;

    friend class QCopClient;
};

#endif
