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

#ifndef QMAILTHREADLISTMODEL_H
#define QMAILTHREADLISTMODEL_H

#include <QAbstractListModel>
#include "qmailthreadkey.h"
#include "qmailthreadsortkey.h"

class QMailThreadListModelPrivate;

class QMF_EXPORT QMailThreadListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles
    {

        ThreadSubjectTextRole = Qt::UserRole,
        ThreadPreviewTextRole,
        ThreadUnreadCountRole,
        ThreadMessageCountRole,
        ThreadSendersRole,
        ThreadLastDateRole,
        ThreadStartedDateRole,
        ThreadIdRole
    };

    QMailThreadListModel(QObject *parent = 0);
    virtual ~QMailThreadListModel();

    int rowCount(const QModelIndex& index = QModelIndex()) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    QMailThreadKey key() const;
    void setKey(const QMailThreadKey& key);

    QMailThreadSortKey sortKey() const;
    void setSortKey(const QMailThreadSortKey& sortKey);

    QMailThreadId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailThreadId& id) const;

    const QMailThreadIdList& ids() const;

    bool synchronizeEnabled() const;
    void setSynchronizeEnabled(bool val);

signals:
   void modelChanged();

private slots:
    void threadsAdded(const QMailThreadIdList& ids);
    void threadsUpdated(const QMailThreadIdList& ids);
    void threadsRemoved(const QMailThreadIdList& ids);

private:
    void fullRefresh();

private:
    QMailThreadListModelPrivate* d;
    friend class tst_QMail_ListModels;
};

#endif // QMAILTHREADLISTMODEL_H
