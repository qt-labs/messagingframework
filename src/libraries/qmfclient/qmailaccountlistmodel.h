/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILACCOUNTLISTMODEL_H
#define QMAILACCOUNTLISTMODEL_H

#include <QAbstractListModel>
#include "qmailaccountkey.h"
#include "qmailaccountsortkey.h"

class QMailAccountListModelPrivate;

class QMF_EXPORT QMailAccountListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        NameTextRole = Qt::UserRole,
        MessageTypeRole,
        MessageSourcesRole,
        MessageSinksRole
    };

public:
    QMailAccountListModel(QObject* parent = Q_NULLPTR);
    virtual ~QMailAccountListModel();

    int rowCount(const QModelIndex& index = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QMailAccountKey key() const;
    void setKey(const QMailAccountKey& key);

    QMailAccountSortKey sortKey() const;
    void setSortKey(const QMailAccountSortKey& sortKey);

    QMailAccountId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailAccountId& id) const;

    bool synchronizeEnabled() const;
    void setSynchronizeEnabled(bool val);

private Q_SLOTS:
    void accountsAdded(const QMailAccountIdList& ids);
    void accountsUpdated(const QMailAccountIdList& ids);
    void accountsRemoved(const QMailAccountIdList& ids);

private:
    void fullRefresh();

private:
    QMailAccountListModelPrivate* d;
    friend class tst_QMail_ListModels;
};

#endif
