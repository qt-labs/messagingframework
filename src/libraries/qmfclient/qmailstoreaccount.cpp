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

#include "qmailstoreaccount.h"
#include "qmaillog.h"

QMailAccountManager::QMailAccountManager(QObject* parent)
    : QObject(parent)
{
}

QMailAccountManager::~QMailAccountManager()
{
}

int QMailAccountManager::countAccounts(const QMailAccountKey &key) const
{
    return queryAccounts(key).count();
}

void QMailAccountManager::addMessageSource(QMailAccount *account,
                                           const QString &source) const
{
    account->addMessageSource(source);
}

void QMailAccountManager::addMessageSink(QMailAccount *account,
                                         const QString &sink) const
{
    account->addMessageSink(sink);
}

bool QMailAccountManager::customFieldsModified(const QMailAccount &account) const
{
    return account.customFieldsModified();
}

void QMailAccountManager::setCustomFieldsModified(QMailAccount *account,
                                                  bool set) const
{
    account->setCustomFieldsModified(set);
}

void QMailAccountManager::setModified(QMailAccountConfiguration *config,
                                      bool set) const
{
    config->setModified(set);
}

QMailAccountManager *QMailAccountManager::newManager(QObject *parent)
{
#ifdef QMF_ACCOUNT_MANAGER_CLASS
    return new QMF_ACCOUNT_MANAGER_CLASS(parent);
#else
    Q_UNUSED(parent);
    return nullptr;
#endif
}
