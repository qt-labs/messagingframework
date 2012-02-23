/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILACTIONFWD_H
#define QMAILACTIONFWD_H
#include "qmailglobal.h"
#include "qmailipc.h"
#include <QPair>

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
    ProtocolRequestRequestType
};

typedef quint64 QMailActionId;
typedef QPair<QMailActionId, QMailServerRequestType> QMailActionData;
typedef QList<QMailActionData> QMailActionDataList;

Q_DECLARE_USER_METATYPE_ENUM(QMailServerRequestType)

Q_DECLARE_METATYPE(QMailActionData)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailActionData, QMailActionData)

Q_DECLARE_METATYPE(QMailActionDataList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailActionDataList, QMailActionDataList)

#endif
