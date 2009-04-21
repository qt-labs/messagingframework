/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGETHREADEDMODEL_H
#define QMAILMESSAGETHREADEDMODEL_H

#include "qmailmessagemodelbase.h"


class QMailMessageThreadedModelPrivate;

class QTOPIAMAIL_EXPORT QMailMessageThreadedModel : public QMailMessageModelBase
{
    Q_OBJECT 

public:
    QMailMessageThreadedModel(QObject* parent = 0);
    virtual ~QMailMessageThreadedModel();

    int rootRow(const QModelIndex& index) const;

    QModelIndex index(int row, int column = 0, const QModelIndex &idx = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &idx) const;

    QModelIndex generateIndex(int row, int column, void *ptr);

protected:
    QMailMessageModelImplementation *impl();
    const QMailMessageModelImplementation *impl() const;

private:
    QMailMessageThreadedModelPrivate* d;
};

#endif
