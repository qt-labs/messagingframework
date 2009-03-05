/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <QContactModel>
#include <QStandardItemModel>

class QContact;
class QMailMessageId;

class MessageModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit MessageModel(QObject* parent = 0);
    virtual ~MessageModel();

    void setContact(const QContact&);

    bool isEmpty() const;

    QMailMessageId messageId(const QModelIndex& index);
};

#endif
