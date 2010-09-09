/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGEREMOVALRECORD_H
#define QMAILMESSAGEREMOVALRECORD_H

#include "qmailid.h"
#include <QString>
#include <QSharedData>
#include <QList>

class QMailMessageRemovalRecordPrivate;

class QMF_EXPORT QMailMessageRemovalRecord
{
public:
    QMailMessageRemovalRecord();
    QMailMessageRemovalRecord(const QMailAccountId& parentAccountId,
                              const QString& serveruid,
                              const QMailFolderId& parentFolderId = QMailFolderId());
    QMailMessageRemovalRecord(const QMailMessageRemovalRecord& other);
    virtual ~QMailMessageRemovalRecord();

    QMailMessageRemovalRecord& operator=(const QMailMessageRemovalRecord& other);

    QMailAccountId parentAccountId() const;
    void setParentAccountId(const QMailAccountId& id);

    QString serverUid() const;
    void setServerUid(const QString& serverUid);

    QMailFolderId parentFolderId() const;
    void setParentFolderId(const QMailFolderId& id);

private:
    QSharedDataPointer<QMailMessageRemovalRecordPrivate> d;

private:
    friend class QMailStore;
    friend class QMailStorePrivate;

};

typedef QList<QMailMessageRemovalRecord> QMailMessageRemovalRecordList;

#endif
