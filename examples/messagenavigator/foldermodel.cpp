/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "foldermodel.h"
#include <QIcon>
#include <QMailStore>
#include <QMailAccountMessageSet>


FolderModel::FolderModel(QObject *parent)
    : QMailMessageSetModel(parent)
{
    // Add an entry for each account, that will maintain its own tree of folders
    foreach (const QMailAccountId &id, QMailStore::instance()->queryAccounts()) 
        append(new QMailAccountMessageSet(this, id, true));
}

FolderModel::~FolderModel()
{
}

QVariant FolderModel::data(QMailMessageSet* item, int role, int column) const
{
    if (role == Qt::DecorationRole) {
        if (qobject_cast<QMailAccountMessageSet*>(item)) {
            // This item is an account message set
            return QIcon(":icon/qtmail/account");
        } else {
            // This item is a folder message set
            return QIcon(":icon/folder");
        }
    } else {
        return QMailMessageSetModel::data(item, role, column);
    }
}
// end-data

