#include "listfilterplugin.h"
#include <qmaildisconnected.h>
#include <QtPlugin>

Q_EXPORT_PLUGIN2(listfilterplugin,ListFilterPlugin)

QMailStore::ErrorCode ListFilterPlugin::ListContentManager::add(QMailMessage *message, DurabilityRequirement)
{
    qDebug() << "Message Added.";

    // is this message sent directly?
    QMailAccountKey accountKey;
    foreach(QMailAddress recip , message->recipients()) {
        qDebug() << "acnt:" << recip.toString();
        accountKey |= QMailAccountKey::fromAddress(recip.address());
    }

    if ( QMailStore::instance()->countAccounts(accountKey) ) {
        qDebug() << "Message: " << message->subject() << " was sent directly..";
        return QMailStore::NoError; // Don't filter messages sent directly
    }

    qDebug() << "Message: " << message->subject() << " was not sent directly..";

    QString listId(message->listId());
    if (!listId.isEmpty()) {
        QMailFolderIdList folderIds(QMailStore::instance()->queryFolders(QMailFolderKey::displayName(listId)
                                                                              & QMailFolderKey::parentAccountId(message->parentAccountId())));

        if (!folderIds.isEmpty()) {
            QMailFolderId folderId(folderIds.first());
            if (folderId != message->parentFolderId()) {
                qDebug() << "Moving message..";
                QMailDisconnected::moveToFolder(message, folderId);
            } else {
                qDebug() << "Already in correct folder. Doing nothing.";
            }
        } else {
            qDebug() << "Not moving message..";
        }
    }

    return QMailStore::NoError;
}

QString ListFilterPlugin::key() const
{
    return QString("listfilterplugin");
}

ListFilterPlugin::ListContentManager *ListFilterPlugin::create()
{
    return &manager;
}
