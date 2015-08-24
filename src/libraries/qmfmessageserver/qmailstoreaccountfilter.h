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

#ifndef QMAILSTOREACCOUNTFILTER_H
#define QMAILSTOREACCOUNTFILTER_H

#include <qmailglobal.h>
#include <qmailstore.h>


class QMailStoreAccountFilterPrivate;
class QMailStoreEvents;

class MESSAGESERVER_EXPORT QMailStoreAccountFilter : public QObject
{
    Q_OBJECT

public:
    QMailStoreAccountFilter(const QMailAccountId &id);
    ~QMailStoreAccountFilter();

Q_SIGNALS:
    void accountUpdated();
    void accountContentsModified();

    void messagesAdded(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);
    void messagesUpdated(const QMailMessageIdList& ids);
    void messageContentsModified(const QMailMessageIdList& ids);

    void foldersAdded(const QMailFolderIdList& ids);
    void foldersRemoved(const QMailFolderIdList& ids);
    void foldersUpdated(const QMailFolderIdList& ids);
    void folderContentsModified(const QMailFolderIdList& ids);

    void messageRemovalRecordsAdded();
    void messageRemovalRecordsRemoved();

private:
    friend class QMailStoreAccountFilterPrivate;
    friend class QMailStoreEvents;

    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);

    QMailStoreAccountFilterPrivate* d;
};

#endif
