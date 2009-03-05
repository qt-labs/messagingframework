/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILSTOREACCOUNTFILTER_H
#define QMAILSTOREACCOUNTFILTER_H

#include <qmailglobal.h>
#include <qmailstore.h>


class QMailStoreAccountFilterPrivate;
class QMailStoreEvents;

class QTOPIAMAIL_EXPORT QMailStoreAccountFilter : public QObject
{
    Q_OBJECT

public:
    QMailStoreAccountFilter(const QMailAccountId &id);
    ~QMailStoreAccountFilter();

signals:
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

    void connectNotify(const char* signal);
    void disconnectNotify(const char* signal);

    QMailStoreAccountFilterPrivate* d;
};

#endif
