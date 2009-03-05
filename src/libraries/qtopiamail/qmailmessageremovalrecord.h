/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGEREMOVALRECORD_H
#define QMAILMESSAGEREMOVALRECORD_H

#include "qmailid.h"
#include <QString>
#include <QSharedData>
#include <QList>

class QMailMessageRemovalRecordPrivate;

class QTOPIAMAIL_EXPORT QMailMessageRemovalRecord
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
