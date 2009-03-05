/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QCOPAPPLICATIONCHANNEL_H
#define QCOPAPPLICATIONCHANNEL_H

#include "qcopchannel.h"

class QCopApplicationChannelPrivate;
class QCopClient;

class Q_QCOP_EXPORT QCopApplicationChannel : public QCopChannel
{
    Q_OBJECT
public:
    explicit QCopApplicationChannel(QObject *parent=0);
    virtual ~QCopApplicationChannel();

    bool isStartupComplete() const;

Q_SIGNALS:
    void startupComplete();

private:
    QCopApplicationChannelPrivate* d;

    friend class QCopClient;
};

#endif
