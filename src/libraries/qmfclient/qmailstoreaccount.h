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

#ifndef QMAILSTOREACCOUNT_H
#define QMAILSTOREACCOUNT_H

#include "qmailglobal.h"
#include "qmailaccount.h"
#include "qmailaccountconfiguration.h"
#include "qmailaccountkey.h"
#include "qmailaccountsortkey.h"

#include <QObject>

class QMF_EXPORT QMailAccountManager : public QObject
{
    Q_OBJECT
public:
    ~QMailAccountManager();

    virtual QMailAccount account(const QMailAccountId &id) const = 0;
    virtual QMailAccountConfiguration accountConfiguration(const QMailAccountId &id) const = 0;
    virtual QMailAccountIdList queryAccounts(const QMailAccountKey &key,
                                             const QMailAccountSortKey &sortKey = QMailAccountSortKey(),
                                             uint limit = 0, uint offset = 0) const = 0;
    virtual bool addAccount(QMailAccount *account,
                            QMailAccountConfiguration *config) = 0;
    virtual bool removeAccounts(const QMailAccountIdList &ids) = 0;
    virtual bool updateAccount(QMailAccount *account,
                               QMailAccountConfiguration *config) = 0;
    virtual bool updateAccountConfiguration(QMailAccountConfiguration *config) = 0;
    virtual int countAccounts(const QMailAccountKey &key) const;
    virtual void clearContent() = 0;

    static QMailAccountManager* newManager(QObject *parent = nullptr);

signals:
    void accountCreated(QMailAccountId id);
    void accountRemoved(QMailAccountId id);
    void accountUpdated(QMailAccountId id);

protected:
    QMailAccountManager(QObject* parent = nullptr);

    // These methods are private within QMailAccount
    // and friendship is not inherited.
    void addMessageSource(QMailAccount *account, const QString &source) const;
    void addMessageSink(QMailAccount *account, const QString &sink) const;

    bool customFieldsModified(const QMailAccount &account) const;
    void setCustomFieldsModified(QMailAccount *account, bool set) const;
    void setModified(QMailAccountConfiguration *config, bool set) const;
};

#endif
