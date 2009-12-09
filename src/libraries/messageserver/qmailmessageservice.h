/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
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
#include <QWidget>
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


#define QMailMessageServicePluginInterface_iid "com.trolltech.Qtopia.Qtopiamail.QMailMessageServicePluginInterface"
Q_DECLARE_INTERFACE(QMailMessageServicePluginInterface, QMailMessageServicePluginInterface_iid)


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

public slots:
    virtual bool retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    virtual bool retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    virtual bool retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    virtual bool retrieveMessagePart(const QMailMessagePart::Location &partLocation);

    virtual bool retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    virtual bool retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);

    virtual bool retrieveAll(const QMailAccountId &accountId);
    virtual bool exportUpdates(const QMailAccountId &accountId);

    virtual bool synchronize(const QMailAccountId &accountId);

    virtual bool deleteMessages(const QMailMessageIdList &ids);

    virtual bool copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    virtual bool moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    virtual bool flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);

    virtual bool createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    virtual bool renameFolder(const QMailFolderId &folderId, const QString &name);
    virtual bool deleteFolder(const QMailFolderId &folderId);

    virtual bool searchMessages(const QMailMessageKey &filter, const QString& bodyText, const QMailMessageSortKey &sort);

    virtual bool prepareMessages(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &ids);

    virtual bool protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data);

signals:
    void newMessagesAvailable();

    void messagesDeleted(const QMailMessageIdList &ids);
    void messagesCopied(const QMailMessageIdList &ids);
    void messagesMoved(const QMailMessageIdList &ids);
    void messagesFlagged(const QMailMessageIdList &ids);

    void matchingMessageIds(const QMailMessageIdList &ids);

    void messagesPrepared(const QMailMessageIdList &ids);

    void protocolResponse(const QString &response, const QVariant &data);

protected slots:
    void deleteMessages();
    void copyMessages();
    void moveMessages();
    void flagMessages();

protected:
    QMailMessageSource(QMailMessageService *service);

    void notImplemented();
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

public slots:
    virtual bool transmitMessages(const QMailMessageIdList &ids);

signals:
    void messagesTransmitted(const QMailMessageIdList &ids);

protected:
    QMailMessageSink(QMailMessageService *service);

    void notImplemented();

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

public slots:
    virtual bool cancelOperation() = 0;

signals:
    void availabilityChanged(bool available);

    void connectivityChanged(QMailServiceAction::Connectivity connectivity);
    void activityChanged(QMailServiceAction::Activity activity);
    void statusChanged(const QMailServiceAction::Status status);
    void progressChanged(uint progress, uint total);

    void actionCompleted(bool success);

protected:
    void updateStatus(QMailServiceAction::Status::ErrorCode code, 
                      const QString &text = QString(), 
                      const QMailAccountId &accountId = QMailAccountId(),
                      const QMailFolderId &folderId = QMailFolderId(), 
                      const QMailMessageId &messageId = QMailMessageId());

    void updateStatus(int code, 
                      const QString &text = QString(), 
                      const QMailAccountId &accountId = QMailAccountId(),
                      const QMailFolderId &folderId = QMailFolderId(), 
                      const QMailMessageId &messageId = QMailMessageId());

private:
    friend class QMailMessageSource;
    friend class QMailMessageSink;

    QMailMessageService(const QMailMessageService &other);
    const QMailMessageService &operator=(const QMailMessageService &other);
};


class MESSAGESERVER_EXPORT QMailMessageServiceEditor : public QWidget
{
    Q_OBJECT

public:
    QMailMessageServiceEditor();
    virtual ~QMailMessageServiceEditor();

    virtual void displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config) = 0;
    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration *config) = 0;
};


class MESSAGESERVER_EXPORT QMailMessageServiceConfigurator
{
public:
    QMailMessageServiceConfigurator();
    virtual ~QMailMessageServiceConfigurator();

    virtual QString service() const = 0;
    virtual QString displayName() const = 0;

    virtual QStringList serviceConstraints(QMailMessageServiceFactory::ServiceType type) const;

    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type) = 0;
};

#endif
