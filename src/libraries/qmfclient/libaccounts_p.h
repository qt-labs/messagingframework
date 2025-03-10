/****************************************************************************
**
** Copyright (C) 2025 Damien Caliste
** Contact: Damien Caliste <dcaliste@free.fr>
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

#ifndef LIBACCOUNTS_P_H
#define LIBACCOUNTS_P_H

#include "qmailstoreaccount.h"

#include <QSharedPointer>
#include <Accounts/Manager>
#include <Accounts/account.h>

class LibAccountManager : public QMailAccountManager
{
public:
    LibAccountManager(QObject *parent = nullptr);
    ~LibAccountManager();

    QMailAccount account(const QMailAccountId &id) const override;
    QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const override;
    QMailAccountIdList queryAccounts(const QMailAccountKey &key,
                                     const QMailAccountSortKey &sortKey = QMailAccountSortKey(),
                                     uint limit = 0, uint offset = 0) const override;
    bool addAccount(QMailAccount *account,
                    QMailAccountConfiguration *config) override;
    bool removeAccounts(const QMailAccountIdList &ids) override;
    bool updateAccount(QMailAccount *account,
                       QMailAccountConfiguration *config) override;
    bool updateAccountConfiguration(QMailAccountConfiguration *config) override;
    void clearContent() override;

private:
    QSharedPointer<Accounts::Manager> manager;

    void onAccountCreated(Accounts::AccountId id);
    void onAccountRemoved(Accounts::AccountId id);
    void onAccountUpdated(Accounts::AccountId id);
    bool onAccountValid(Accounts::AccountId id) const;
    bool accountValid(Accounts::AccountId id) const;
    QSharedPointer<Accounts::Account> getAccount(const QMailAccountId &id) const;
    void updateAccountCustomFields(QSharedPointer<Accounts::Account>& ssoAccount,
                                   const QMap<QString, QString> &fields);
    bool updateSharedAccount(QMailAccount *account,
                             QMailAccountConfiguration *config);
};

#endif
