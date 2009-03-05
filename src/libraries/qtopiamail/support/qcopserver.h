/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QCOPSERVER_H
#define QCOPSERVER_H

#include <qobject.h>

#if !defined(Q_QCOP_EXPORT)
#if defined(QT_BUILD_QCOP_LIB)
#define Q_QCOP_EXPORT Q_DECL_EXPORT
#else
#define Q_QCOP_EXPORT Q_DECL_IMPORT
#endif
#endif

class QCopServerPrivate;
class QCopClient;

class Q_QCOP_EXPORT QCopServer : public QObject
{
    Q_OBJECT
public:
    QCopServer(QObject *parent = 0);
    ~QCopServer();

protected:
    virtual qint64 activateApplication(const QString& name);
    void applicationExited(qint64 pid);

private:
    QCopServerPrivate *d;

    friend class QCopServerPrivate;
    friend class QCopClient;
};

#endif
