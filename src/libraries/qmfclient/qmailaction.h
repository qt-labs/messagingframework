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

#ifndef QMAILACTIONFWD_H
#define QMAILACTIONFWD_H
#include "qmailglobal.h"
#include "qmailipc.h"
#include "qmailid.h"
#include <QString>
#include <QList>
#include <QSharedDataPointer>

enum QMailServerRequestType
{
    AcknowledgeNewMessagesRequestType,
    TransmitMessagesRequestType,
    RetrieveFolderListRequestType,
    RetrieveMessageListRequestType,
    RetrieveNewMessagesRequestType,
    RetrieveMessagesRequestType,
    RetrieveMessagePartRequestType,
    RetrieveMessageRangeRequestType,
    RetrieveMessagePartRangeRequestType,
    RetrieveAllRequestType,
    ExportUpdatesRequestType,
    SynchronizeRequestType,
    CopyMessagesRequestType,
    MoveMessagesRequestType,
    FlagMessagesRequestType,
    CreateFolderRequestType,
    RenameFolderRequestType,
    DeleteFolderRequestType,
    CancelTransferRequestType,
    DeleteMessagesRequestType,
    SearchMessagesRequestType,
    CancelSearchRequestType,
    ListActionsRequestType,
    ProtocolRequestRequestType,
    MoveFolderRequestType
};

typedef quint64 QMailActionId;

class QMailActionDataPrivate;

class QMF_EXPORT QMailActionData
{
public:
    QMailActionData();
    QMailActionData(QMailActionId id,
                    QMailServerRequestType requestType,
                    uint progressCurrent,
                    uint progressTotal,
                    int errorCode,
                    const QString &text,
                    const QMailAccountId &accountId,
                    const QMailFolderId &folderId,
                    const QMailMessageId &messageId);
    QMailActionData(const QMailActionData& other);
    ~QMailActionData();

    QMailActionId id() const;
    QMailServerRequestType requestType() const;
    uint progressCurrent() const;
    uint progressTotal() const;
    int errorCode() const;
    QString text() const;
    QMailAccountId accountId() const;
    QMailFolderId folderId() const;
    QMailMessageId messageId() const;

    bool operator==(const QMailActionData& other) const;
    bool operator!=(const QMailActionData& other) const;

    const QMailActionData& operator=(const QMailActionData& other);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    QSharedDataPointer<QMailActionDataPrivate> d;
};

typedef QList<QMailActionData> QMailActionDataList;

Q_DECLARE_USER_METATYPE_ENUM(QMailServerRequestType)
Q_DECLARE_USER_METATYPE(QMailActionData)
Q_DECLARE_METATYPE(QMailActionDataList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailActionDataList, QMailActionDataList)

#endif
