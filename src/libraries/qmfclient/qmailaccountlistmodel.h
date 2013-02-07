/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
    QMailAccountListModel(QObject* parent = 0);
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

private slots:
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
