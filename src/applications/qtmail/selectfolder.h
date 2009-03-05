/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/
#ifndef SELECTFOLDER_H
#define SELECTFOLDER_H

#include <QDialog>
#include <QList>
#include <qmailid.h>


class QListWidget;

class SelectFolderDialog : public QDialog
{
    Q_OBJECT

public:
    SelectFolderDialog(const QMailFolderIdList &folderIds, QWidget *parent = 0);
    virtual ~SelectFolderDialog();

    QMailFolderId selectedFolderId() const;

private slots:
    void selected();

private:
    QListWidget *mFolderList;
    QMailFolderIdList mFolderIds;
};

#endif
