#include "listfilterplugin.h"
#include <qmaildisconnected.h>
#include <QtPlugin>

Q_EXPORT_PLUGIN2(listfilterplugin,ListFilterPlugin)

QMailStore::ErrorCode ListFilterPlugin::ListContentManager::add(QMailMessage *message, DurabilityRequirement)
{
    // is this message sent directly?
    QMailAccountKey accountKey;
    foreach(QMailAddress recip , message->recipients())
        accountKey |= QMailAccountKey::fromAddress(recip.address());

    if ( QMailStore::instance()->countAccounts(accountKey) )
        return QMailStore::NoError; // Don't filter messages sent directly

    QString listId(message->listId());
    if (!listId.isEmpty()) {
        QMailFolderIdList folderIds(QMailStore::instance()->queryFolders(QMailFolderKey::displayName(listId)
                                                                              & QMailFolderKey::parentAccountId(message->parentAccountId())));
        if (!folderIds.isEmpty()) {
            QMailFolderId folderId(folderIds.first());
            if (folderId != message->parentFolderId()) {
                QMailDisconnected::moveToFolder(message, folderId);
            }
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
