/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGELISTMODEL_H
#define QMAILMESSAGELISTMODEL_H

#include "qmailmessagemodelbase.h"


class QMailMessageListModelPrivate;

class QTOPIAMAIL_EXPORT QMailMessageListModel : public QMailMessageModelBase
{
    Q_OBJECT 

public:
    QMailMessageListModel(QObject* parent = 0);
    virtual ~QMailMessageListModel();

    QModelIndex index(int row, int column = 0, const QModelIndex &idx = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &idx) const;

    QModelIndex generateIndex(int row, int column, void *ptr);

protected:
    QMailMessageModelImplementation *impl();
    const QMailMessageModelImplementation *impl() const;

private:
    QMailMessageListModelPrivate* d;
};

#endif
