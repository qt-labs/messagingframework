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

#ifndef QMAILMESSAGESERVICE_H
#define QMAILMESSAGESERVICE_H

#include <qmailmessage.h>
#include <qmailserviceaction.h>
#include <qmailstore.h>
#include <QMap>
#include <QObject>
#include <QString>
#include <qfactoryinterface.h>


/* Note: the obvious design for these classes would be that Sink and Source
both inherit virtually from Service, and thus a concrete service could 
inherit from both Source and Sink.  In fac, moc does not work with
virtual inheritance...  
Instead, we will have the service object export the source and sink 
objects that it wishes to make available. */


class QMailAccount;
class QMailAccountConfiguration;

class QMailMessageService;
class QMailMessageServiceConfigurator;


class MESSAGESERVER_EXPORT QMailMessageServiceFactory
{
public:
    enum ServiceType { Any = 0, Source, Sink, Storage };

    static QStringList keys(ServiceType type = Any);

    static bool supports(const QString &key, ServiceType type);
    static bool supports(const QString &key, QMailMessage::MessageType messageType);

    static QMailMessageService *createService(const QString &key, const QMailAccountId &id);
    static QMailMessageServiceConfigurator *createServiceConfigurator(const QString &key);
};


struct MESSAGESERVER_EXPORT QMailMessageServicePluginInterface : public QFactoryInterface
{
    virtual QString key() const = 0;
    virtual bool supports(QMailMessageServiceFactory::ServiceType type) const = 0;
    virtual bool supports(QMailMessage::MessageType messageType) const = 0;

    virtual QMailMessageService *createService(const QMailAccountId &id) = 0;
    virtual QMailMessageServiceConfigurator *createServiceConfigurator();
};


QT_BEGIN_NAMESPACE

#define QMailMessageServicePluginInterface_iid "com.nokia.qmf.QMailMessageServicePluginInterface"
Q_DECLARE_INTERFACE(QMailMessageServicePluginInterface, QMailMessageServicePluginInterface_iid)

QT_END_NAMESPACE

class MESSAGESERVER_EXPORT QMailMessageServicePlugin : public QObject, public QMailMessageServicePluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QMailMessageServicePluginInterface:QFactoryInterface)

public:
    QMailMessageServicePlugin();
    ~QMailMessageServicePlugin();

    virtual QStringList keys() const;
};


class QMailMessageSourcePrivate;

class MESSAGESERVER_EXPORT QMailMessageSource : public QObject
{
    Q_OBJECT

public:
    virtual ~QMailMessageSource();

    virtual QMailStore::MessageRemovalOption messageRemovalOption() const;

public Q_SLOTS:
    virtual bool retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    virtual bool retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending, quint64 action);

    virtual bool retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    virtual bool retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort, quint64 action);

    virtual bool retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort);
    virtual bool retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort, quint64 action);

    virtual bool retrieveNewMessages(const QMailAccountId &accountId, const QMailFolderIdList &folderIds);
    virtual bool retrieveNewMessages(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, quint64 action);

    virtual bool retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    virtual bool retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec, quint64 action);
    virtual bool retrieveMessagePart(const QMailMessagePart::Location &partLocation);
    virtual bool retrieveMessagePart(const QMailMessagePart::Location &partLocation, quint64 action);

    virtual bool retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    virtual bool retrieveMessageRange(const QMailMessageId &messageId, uint minimum, quint64 action);
    virtual bool retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);
    virtual bool retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum, quint64 action);

    virtual bool retrieveAll(const QMailAccountId &accountId);
    virtual bool retrieveAll(const QMailAccountId &accountId, quint64 action);
    virtual bool exportUpdates(const QMailAccountId &accountId);
    virtual bool exportUpdates(const QMailAccountId &accountId, quint64 action);

    virtual bool synchronize(const QMailAccountId &accountId);
    virtual bool synchronize(const QMailAccountId &accountId, quint64 action);

    virtual bool deleteMessages(const QMailMessageIdList &ids);
    virtual bool deleteMessages(const QMailMessageIdList &ids, quint64 action);

    virtual bool copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    virtual bool copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId, quint64 action);
    virtual bool moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    virtual bool moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId, quint64 action);
    virtual bool flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);
    virtual bool flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask, quint64 action);

    virtual bool createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    virtual bool createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId, quint64 action);
    virtual bool createStandardFolders(const QMailAccountId &accountId);
    virtual bool createStandardFolders(const QMailAccountId &accountId, quint64 action);
    virtual bool renameFolder(const QMailFolderId &folderId, const QString &name);
    virtual bool renameFolder(const QMailFolderId &folderId, const QString &name, quint64 action);
    virtual bool deleteFolder(const QMailFolderId &folderId);
    virtual bool deleteFolder(const QMailFolderId &folderId, quint64 action);
    virtual bool moveFolder(const QMailFolderId &folderId, const QMailFolderId &newParentId);
    virtual bool moveFolder(const QMailFolderId &folderId, const QMailFolderId &newParentId, quint64 action);

    virtual bool searchMessages(const QMailMessageKey &filter, const QString& bodyText, quint64 limit, const QMailMessageSortKey &sort);
    virtual bool searchMessages(const QMailMessageKey &filter, const QString& bodyText, quint64 limit, const QMailMessageSortKey &sort, quint64 action);
    virtual bool searchMessages(const QMailMessageKey &filter, const QString& bodyText, const QMailMessageSortKey &sort);
    virtual bool searchMessages(const QMailMessageKey &filter, const QString& bodyText, const QMailMessageSortKey &sort, quint64 action);
    virtual bool countMessages(const QMailMessageKey &filter, const QString& bodyText);
    virtual bool countMessages(const QMailMessageKey &filter, const QString& bodyText, quint64 action);
    virtual bool cancelSearch();
    virtual bool cancelSearch(quint64 action);

    virtual bool prepareMessages(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &ids);
    virtual bool prepareMessages(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &ids, quint64 action);

    virtual bool protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data);
    virtual bool protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data, quint64 action);

Q_SIGNALS:
    void newMessagesAvailable();
    void newMessagesAvailable(quint64 action);

    void messagesDeleted(const QMailMessageIdList &ids);
    void messagesDeleted(const QMailMessageIdList &ids, quint64 action);
    void messagesCopied(const QMailMessageIdList &ids);
    void messagesCopied(const QMailMessageIdList &ids, quint64 action);
    void messagesMoved(const QMailMessageIdList &ids);
    void messagesMoved(const QMailMessageIdList &ids, quint64 action);
    void messagesFlagged(const QMailMessageIdList &ids);
    void messagesFlagged(const QMailMessageIdList &ids, quint64 action);

    void matchingMessageIds(const QMailMessageIdList &ids);
    void matchingMessageIds(const QMailMessageIdList &ids, quint64 action);
    void remainingMessagesCount(uint);
    void remainingMessagesCount(uint, quint64 action);
    void messagesCount(uint);
    void messagesCount(uint, quint64 action);

    void messagesPrepared(const QMailMessageIdList &ids);
    void messagesPrepared(const QMailMessageIdList &ids, quint64 action);

    void protocolResponse(const QString &response, const QVariant &data);
    void protocolResponse(const QString &response, const QVariant &data, quint64 action);

protected Q_SLOTS:
    void deleteMessages();
    void copyMessages();
    void moveMessages();
    void flagMessages();

protected:
    QMailMessageSource(QMailMessageService *service);

    void notImplemented();
    void notImplemented(quint64 action);
    bool modifyMessageFlags(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);

private:
    QMailMessageSource();
    QMailMessageSource(const QMailMessageSource &other);
    const QMailMessageSource &operator=(const QMailMessageSource &other);

    QMailMessageSourcePrivate *d;
};

class QMailMessageSinkPrivate;

class MESSAGESERVER_EXPORT QMailMessageSink : public QObject
{
    Q_OBJECT

public:
    ~QMailMessageSink();

public Q_SLOTS:
    virtual bool transmitMessages(const QMailMessageIdList &ids);
    virtual bool transmitMessages(const QMailMessageIdList &ids, quint64 action);

Q_SIGNALS:
    void messagesTransmitted(const QMailMessageIdList &ids);
    void messagesTransmitted(const QMailMessageIdList &ids, quint64 action);
    void messagesFailedTransmission(const QMailMessageIdList &ids, QMailServiceAction::Status::ErrorCode);
    void messagesFailedTransmission(const QMailMessageIdList &ids, QMailServiceAction::Status::ErrorCode, quint64 action);

protected:
    QMailMessageSink(QMailMessageService *service);

    void notImplemented();
    void notImplemented(quint64 action);

private:
    QMailMessageSink();
    QMailMessageSink(const QMailMessageSink &other);
    const QMailMessageSink &operator=(const QMailMessageSink &other);

    QMailMessageSinkPrivate *d;
};


class MESSAGESERVER_EXPORT QMailMessageService : public QObject
{
    Q_OBJECT

public:
    QMailMessageService();
    virtual ~QMailMessageService();

    virtual QString service() const = 0;
    virtual QMailAccountId accountId() const = 0;

    virtual bool hasSource() const;
    virtual QMailMessageSource &source() const;

    virtual bool hasSink() const;
    virtual QMailMessageSink &sink() const;

    virtual bool available() const = 0;

    virtual bool requiresReregistration() const { return false; }
    virtual bool usesConcurrentActions() const { return false; }

    static int reservePushConnections(int connections);
    static void releasePushConnections(int connections);

public Q_SLOTS:
    virtual bool cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text) = 0;
    virtual bool cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text, quint64 action);
    bool cancelOperation() { return cancelOperation(QMailServiceAction::Status::ErrCancel, tr("Cancelled by user")); }
    bool cancelOperation(quint64 action) { return cancelOperation(QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"), action); }

Q_SIGNALS:
    void availabilityChanged(bool available);

    void connectivityChanged(QMailServiceAction::Connectivity connectivity);
    void activityChanged(QMailServiceAction::Activity activity);
    void activityChanged(QMailServiceAction::Activity activity, quint64 action);
    void statusChanged(const QMailServiceAction::Status status);
    void statusChanged(const QMailServiceAction::Status status, quint64 action);
    void progressChanged(uint progress, uint total);
    void progressChanged(uint progress, uint total, quint64 action);

    void actionCompleted(bool success);
    void actionCompleted(bool success, quint64 action);

protected:
    void updateStatus(QMailServiceAction::Status::ErrorCode code, 
                      const QString &text = QString(), 
                      const QMailAccountId &accountId = QMailAccountId(),
                      const QMailFolderId &folderId = QMailFolderId(), 
                      const QMailMessageId &messageId = QMailMessageId(),
                      quint64 action = 0);

    void updateStatus(int code, 
                      const QString &text = QString(), 
                      const QMailAccountId &accountId = QMailAccountId(),
                      const QMailFolderId &folderId = QMailFolderId(), 
                      const QMailMessageId &messageId = QMailMessageId(),
                      quint64 action = 0);

private:
    friend class QMailMessageSource;
    friend class QMailMessageSink;

    QMailMessageService(const QMailMessageService &other);
    const QMailMessageService &operator=(const QMailMessageService &other);
};

#ifndef QMF_NO_WIDGETS
#include <QWidget>
class MESSAGESERVER_EXPORT QMailMessageServiceEditor : public QWidget
{
    Q_OBJECT

public:
    QMailMessageServiceEditor();
    virtual ~QMailMessageServiceEditor();

    virtual void displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config) = 0;
    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration *config) = 0;
};
#endif

class MESSAGESERVER_EXPORT QMailMessageServiceConfigurator
{
public:
    QMailMessageServiceConfigurator();
    virtual ~QMailMessageServiceConfigurator();

    virtual QString service() const = 0;
    virtual QString displayName() const = 0;

    virtual QStringList serviceConstraints(QMailMessageServiceFactory::ServiceType type) const;

#ifndef QMF_NO_WIDGETS
    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type) = 0;
#endif
};

#endif
