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

#include "foldermodel.h"
#include <qmailaccount.h>
#include <qmailfolder.h>
#include <qmailstore.h>
#include <QApplication>
#include <QTimer>
#include "qtmailnamespace.h"

using QMailDataComparator::Includes;
using QMailDataComparator::Excludes;

FolderModel::FolderModel(QObject *parent)
    : QMailMessageSetModel(parent)
{
}

FolderModel::~FolderModel()
{
}

QVariant FolderModel::data(QMailMessageSet *item, int role, int column) const
{
    if (item) {
        if (role == FolderIconRole) {
            return itemIcon(item);
        } else if (role == FolderStatusRole) {
            return itemStatus(item);
        } else if (role == FolderStatusDetailRole) {
            return itemStatusDetail(item);
        } else if (role == FolderIdRole) {
            return itemFolderId(item);
        }

        return QMailMessageSetModel::data(item, role, column);
    }

    return QVariant();
}

QString FolderModel::excessIndicator()
{
    return QLatin1String("*");
}

void FolderModel::appended(QMailMessageSet *item)
{
    QMailMessageSetModel::appended(item);

    // Determine an initial status for this item
    scheduleUpdate(item);
}

void FolderModel::updated(QMailMessageSet *item)
{
    QMailMessageSetModel::updated(item);

    // See if the status has changed for this item
    scheduleUpdate(item);
}

void FolderModel::removed(QMailMessageSet *item)
{
    QMailMessageSetModel::removed(item);

    updatedItems.removeAll(item);
}

QIcon FolderModel::itemIcon(QMailMessageSet *item) const
{
    if (qobject_cast<QMailFolderMessageSet*>(item)) {
        return Qtmail::icon(QLatin1String("folder"));
    } else if (qobject_cast<QMailAccountMessageSet*>(item)) {
        return Qtmail::icon(QLatin1String("accountfolder"));
    } else if (qobject_cast<QMailFilterMessageSet*>(item)) {
        return Qtmail::icon(QLatin1String("search"));
    }

    return QIcon();
}

QString FolderModel::itemStatus(QMailMessageSet *item) const
{
    QMap<QMailMessageSet*, StatusText>::const_iterator it = statusMap.find(item);
    if (it != statusMap.end())
        return it->first;

    return QString();
}

QString FolderModel::itemStatusDetail(QMailMessageSet *item) const
{
    QMap<QMailMessageSet*, StatusText>::const_iterator it = statusMap.find(item);
    if (it != statusMap.end())
        return it->second;

    return QString();
}

FolderModel::StatusText FolderModel::itemStatusText(QMailMessageSet *item) const
{
    if (QMailFolderMessageSet *folderItem = qobject_cast<QMailFolderMessageSet*>(item)) {
        return folderStatusText(folderItem);
    } else if (QMailAccountMessageSet *accountItem = qobject_cast<QMailAccountMessageSet*>(item)) {
        return accountStatusText(accountItem);
    } else if (QMailFilterMessageSet *filterItem = qobject_cast<QMailFilterMessageSet*>(item)) {
        return filterStatusText(filterItem);
    }

    return qMakePair(QString(), QString());
}

QString FolderModel::formatCounts(int total, int unread, bool excessTotal, bool excessUnread)
{
    QString countStr;

    if (total || excessTotal || excessUnread) {
        if (unread || excessUnread) {
            QString unreadIndicator(excessUnread ? FolderModel::excessIndicator() : QLatin1String(""));
            QString totalIndicator(excessTotal ? FolderModel::excessIndicator() : QLatin1String(""));

            if (QApplication::isRightToLeft())
                countStr.append(QString::fromLatin1("%1%2/%3%4").arg(total).arg(totalIndicator).arg(unread).arg(unreadIndicator));
            else
                countStr.append(QString::fromLatin1("%1%2/%3%4").arg(unread).arg(unreadIndicator).arg(total).arg(totalIndicator));
        } else {
            countStr.append(QString::fromLatin1("%1%2").arg(total).arg(excessTotal ? FolderModel::excessIndicator() : QLatin1String("")));
        }
    }

    return countStr;
}

QString FolderModel::describeFolderCount(int total, int subTotal, SubTotalType type)
{
    QString desc(QString::number(total));

    if (total && subTotal) {
        if (type == New) {
            desc += tr(" (%n new)", "%1 = number of new messages", subTotal);
        } else if (type == Unsent) {
            desc += tr(" (%n unsent)", "%1 = number of unsent messages", subTotal);
        } else if (type == Unread) {
            desc += tr(" (%n unread)", "%1 = number of unread messages", subTotal);
        }
    }

    return desc;
}

QMailMessageKey FolderModel::unreadKey()
{
    // Both 'read' and 'read-elsewhere' mean !unread
    return (QMailMessageKey::status(QMailMessage::Read, Excludes) &
            QMailMessageKey::status(QMailMessage::ReadElsewhere, Excludes));
}

FolderModel::StatusText FolderModel::folderStatusText(QMailFolderMessageSet *item) const
{
    QString status, detail;

    if (QMailStore* store = QMailStore::instance()) {
        int inclusiveTotal = 0;
        int inclusiveUnreadTotal = 0;

        // Find the total and unread total for this folder
        QMailMessageKey itemKey = item->messageKey();
        int total = store->countMessages(itemKey);
        int unreadTotal = store->countMessages(itemKey & unreadKey());

        // Determine whether there are messages lower in the hierarchy
        QMailMessageKey inclusiveKey = item->descendantsMessageKey();
        inclusiveTotal = total + store->countMessages(inclusiveKey);

        if (inclusiveTotal > total) {
            inclusiveUnreadTotal = unreadTotal + store->countMessages(inclusiveKey & unreadKey());
        }

        detail = describeFolderCount(total, unreadTotal, Unread);
        status = formatCounts(total, unreadTotal, (inclusiveTotal > total), (inclusiveUnreadTotal > unreadTotal));
    }

    return qMakePair(status, detail);
}

FolderModel::StatusText FolderModel::accountStatusText(QMailAccountMessageSet *item) const
{
    QString status, detail;

    if (QMailStore* store = QMailStore::instance()) {
        QMailMessageKey itemKey = item->messageKey();
        int total = store->countMessages(itemKey);

        if (total) {
            // Find the unread total for this account
            int unreadTotal = store->countMessages(itemKey & unreadKey());

            // See if there are 'new' messages for this account
            int subTotal = store->countMessages(itemKey & QMailMessageKey::status(QMailMessage::New, Includes));
            if (subTotal) {
                detail = describeFolderCount(total, subTotal, New);
            } else {
                detail = formatCounts(total, unreadTotal, false, false);
            }

            status = formatCounts(total, unreadTotal, false, false);
        } else {
            detail = QString::number(0);
        }
    }

    return qMakePair(status, detail);
}

FolderModel::StatusText FolderModel::filterStatusText(QMailFilterMessageSet *item) const
{
    QString status, detail;

    if (QMailStore* store = QMailStore::instance()) {
        QMailMessageKey itemKey = item->messageKey();
        int total = store->countMessages(itemKey);

        if (total) {
            // Find the unread total for this set
            int unreadTotal = store->countMessages(itemKey & unreadKey());

            detail = describeFolderCount(total, unreadTotal);
            status = formatCounts(total, unreadTotal, false, false);
        } else {
            detail = QString::number(0);
        }
    }

    return qMakePair(status, detail);
}

void FolderModel::scheduleUpdate(QMailMessageSet *item)
{
    if (updatedItems.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(processUpdatedItems()));
    } else if (updatedItems.contains(item)) {
        return;
    }

    updatedItems.append(item);
}

void FolderModel::processUpdatedItems()
{
    // Note: throughput can be increased at a cost to interactivity by increasing batchSize:
    const int batchSize = 1;

    // Only process a small number before returning to the event loop
    int count = 0;
    while (!updatedItems.isEmpty() && (count < batchSize)) {
        QMailMessageSet *item = updatedItems.takeFirst();

        FolderModel::StatusText text = itemStatusText(item);
        if (text != statusMap[item]) {
            statusMap[item] = text;
            emit dataChanged(item->modelIndex(), item->modelIndex());
        }

        ++count;
    }

    if (!updatedItems.isEmpty())
        QTimer::singleShot(0, this, SLOT(processUpdatedItems()));
}

