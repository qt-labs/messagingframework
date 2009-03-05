/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef PopClient_H
#define PopClient_H

#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <qlist.h>
#include <qtimer.h>
#include <qmailaccountconfiguration.h>
#include <qmailmessage.h>
#include <qmailmessageclassifier.h>
#include <qmailmessageserver.h>
#include <qmailtransport.h>

class LongStream;
class QMailTransport;
class QMailAccount;

typedef QMap<QString, QMailMessageId> SelectionMap;

class PopClient : public QObject
{
    Q_OBJECT

public:
    PopClient(QObject* parent);
    ~PopClient();

    QMailMessage::MessageType messageType() const;

    void setAccount(const QMailAccountId &accountId);
    QMailAccountId account() const;

    void newConnection();
    void closeConnection();
    void setOperation(QMailRetrievalAction::RetrievalSpecification spec);
    void setDeleteOperation();
    void setSelectedMails(const SelectionMap& data);
    void checkForNewMessages();
    void cancelTransfer();

signals:
    void errorOccurred(int, const QString &);
    void errorOccurred(QMailServiceAction::Status::ErrorCode, const QString &);
    void updateStatus(const QString &);

    void messageActionCompleted(const QString &uid);

    void progressChanged(uint, uint);
    void retrievalCompleted();

    void allMessagesReceived();

protected slots:
    void connected(QMailTransport::EncryptType encryptType);
    void transportError(int, QString msg);

    void connectionInactive();
    void incomingData();

private:
    void deactivateConnection();
    int nextMsgServerPos();
    int msgPosFromUidl(QString uidl);
    uint getSize(int pos);
    void uidlIntegrityCheck();
    void createMail();
    void sendCommand(const char *data, int len = -1);
    void sendCommand(const QString& cmd);
    void sendCommand(const QByteArray& cmd);
    QString readResponse();
    void processResponse(const QString &response);
    void nextAction();
    void retrieveOperationCompleted();
    void messageProcessed(const QString &uid);

    void operationFailed(int code, const QString &text);
    void operationFailed(QMailServiceAction::Status::ErrorCode code, const QString &text);

private:
    enum TransferStatus
    {
        Init, CapabilityTest, Capabilities, 
        StartTLS, TLS, Connected, Auth, 
        RequestUids, Uidl, UidList,
        RequestSizes, List, SizeList,
        RequestMessage, Retr, MessageData,
        DeleteMessage, Dele,
        Done, Quit, Exit
    };

    QMailAccountConfiguration config;
    QTimer inactiveTimer;
    TransferStatus status;
    int messageCount;
    bool selected;
    bool deleting;
    QString message;
    SelectionMap selectionMap;
    SelectionMap::ConstIterator selectionItr;
    uint mailSize;
    uint headerLimit;

    QMap<QString, int> serverUidNumber;
    QMap<int, QString> serverUid;
    QMap<int, uint> serverSize;

    QString messageUid;
    QStringList newUids;
    QStringList obsoleteUids;
    LongStream *dataStream;

    QMailTransport *transport;

    QString retrieveUid;

    SelectionMap completionList;

    uint progress;
    uint total;

    // RetrievalMap maps uid -> ((units, bytes) to be retrieved, percentage retrieved)
    typedef QMap<QString, QPair< QPair<uint, uint>, uint> > RetrievalMap;
    RetrievalMap retrievalSize;
    uint progressRetrievalSize;
    uint totalRetrievalSize;

    QMailMessageClassifier classifier;

    QStringList capabilities;
    QList<QByteArray> authCommands;
};

#endif
