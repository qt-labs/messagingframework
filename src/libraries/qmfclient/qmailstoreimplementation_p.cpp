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

#include "qmailstoreimplementation_p.h"
#include <qmailnamespace.h>

QMailStore::InitializationState QMailStoreImplementationBase::initState = QMailStore::Uninitialized;

QMailStoreImplementationBase::QMailStoreImplementationBase(QMailStore* parent)
    : q(parent),
      errorCode(QMailStore::NoError)
{
    Q_ASSERT(q);
}

QMailStoreImplementationBase::~QMailStoreImplementationBase()
{
}

void QMailStoreImplementationBase::initialize()
{
    initState = (initStore() ? QMailStore::Initialized : QMailStore::InitializationFailed);
}

QMailStore::InitializationState QMailStoreImplementationBase::initializationState()
{
    return initState;
}

QMailStore::ErrorCode QMailStoreImplementationBase::lastError() const
{
    return errorCode;
}

void QMailStoreImplementationBase::setLastError(QMailStore::ErrorCode code) const
{
    if (initState == QMailStore::InitializationFailed) {
        // Enforce the error code to be this if we can't init:
        code = QMailStore::StorageInaccessible;
    }

    if (errorCode != code) {
        errorCode = code;

        if (errorCode != QMailStore::NoError) {
            q->emitErrorNotification(errorCode);
        }
    }
}

QMailStoreImplementation::QMailStoreImplementation(QMailStore* parent)
    : QMailStoreImplementationBase(parent)
    , QMailStoreNotifier(parent)
{
}


QMailStoreNullImplementation::QMailStoreNullImplementation(QMailStore* parent)
    : QMailStoreImplementation(parent)
{
    setLastError(QMailStore::StorageInaccessible);
}

void QMailStoreNullImplementation::clearContent()
{
}

bool QMailStoreNullImplementation::addAccount(QMailAccount *, QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addFolder(QMailFolder *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addMessages(const QList<QMailMessage *> &, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addMessages(const QList<QMailMessageMetaData *> &, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addThread(QMailThread *, QMailThreadIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeAccounts(const QMailAccountKey &, QMailAccountIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeFolders(const QMailFolderKey &, QMailStore::MessageRemovalOption, QMailFolderIdList *, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeMessages(const QMailMessageKey &, QMailStore::MessageRemovalOption, QMailMessageIdList *, QMailThreadIdList*, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeThreads(const QMailThreadKey &, QMailStore::MessageRemovalOption ,
                                                              QMailThreadIdList *, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateAccount(QMailAccount *, QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateAccountConfiguration(QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateFolder(QMailFolder *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessagesMetaData(const QMailMessageKey &, const QMailMessageKey::Properties &, const QMailMessageMetaData &, QMailMessageIdList *, QMailThreadIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessagesMetaData(const QMailMessageKey &, quint64, bool, QMailMessageIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateThread(QMailThread *, QMailThreadIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::ensureDurability()
{
    return false;
}

void QMailStoreNullImplementation::lock()
{
}

void QMailStoreNullImplementation::unlock()
{
}

bool QMailStoreNullImplementation::purgeMessageRemovalRecords(const QMailAccountId &, const QStringList &)
{
    return false;
}

int QMailStoreNullImplementation::countAccounts(const QMailAccountKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countFolders(const QMailFolderKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countMessages(const QMailMessageKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countThreads(const QMailThreadKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::sizeOfMessages(const QMailMessageKey &) const
{
    return 0;
}

QMailAccountIdList QMailStoreNullImplementation::queryAccounts(const QMailAccountKey &, const QMailAccountSortKey &, uint, uint) const
{
    return QMailAccountIdList();
}

QMailFolderIdList QMailStoreNullImplementation::queryFolders(const QMailFolderKey &, const QMailFolderSortKey &, uint, uint) const
{
    return QMailFolderIdList();
}

QMailMessageIdList QMailStoreNullImplementation::queryMessages(const QMailMessageKey &, const QMailMessageSortKey &, uint, uint) const
{
    return QMailMessageIdList();
}

QMailThreadIdList QMailStoreNullImplementation::queryThreads(const QMailThreadKey &, const QMailThreadSortKey &, uint, uint) const
{
    return QMailThreadIdList();
}

QMailAccount QMailStoreNullImplementation::account(const QMailAccountId &) const
{
    return QMailAccount();
}

QMailAccountConfiguration QMailStoreNullImplementation::accountConfiguration(const QMailAccountId &) const
{
    return QMailAccountConfiguration();
}

QMailFolder QMailStoreNullImplementation::folder(const QMailFolderId &) const
{
    return QMailFolder();
}

QMailMessage QMailStoreNullImplementation::message(const QMailMessageId &) const
{
    return QMailMessage();
}

QMailMessage QMailStoreNullImplementation::message(const QString &, const QMailAccountId &) const
{
    return QMailMessage();
}

QMailThread QMailStoreNullImplementation::thread(const QMailThreadId &) const
{
    return QMailThread();
}

QMailMessageMetaData QMailStoreNullImplementation::messageMetaData(const QMailMessageId &) const
{
    return QMailMessageMetaData();
}

QMailMessageMetaData QMailStoreNullImplementation::messageMetaData(const QString &, const QMailAccountId &) const
{
    return QMailMessageMetaData();
}

QMailMessageMetaDataList QMailStoreNullImplementation::messagesMetaData(const QMailMessageKey &, const QMailMessageKey::Properties &, QMailStore::ReturnOption) const
{
    return QMailMessageMetaDataList();
}

QMailThreadList QMailStoreNullImplementation::threads(const QMailThreadKey &, QMailStore::ReturnOption) const
{
    return QMailThreadList();
}

QMailMessageRemovalRecordList QMailStoreNullImplementation::messageRemovalRecords(const QMailAccountId &, const QMailFolderId &) const
{
    return QMailMessageRemovalRecordList();
}

bool QMailStoreNullImplementation::registerAccountStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::accountStatusMask(const QString &) const
{
    return 0;
}

bool QMailStoreNullImplementation::registerFolderStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::folderStatusMask(const QString &) const
{
    return 0;
}

bool QMailStoreNullImplementation::registerMessageStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::messageStatusMask(const QString &) const
{
    return 0;
}

QMap<QString, QString> QMailStoreNullImplementation::messageCustomFields(const QMailMessageId &)
{
    return QMap<QString, QString>();
}

bool QMailStoreNullImplementation::initStore()
{
    return false;
}

