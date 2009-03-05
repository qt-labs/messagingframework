/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "selectfolder.h"
#ifdef QMAIL_QTOPIA
#include <QtopiaApplication>
#endif
#include <QListWidget>
#include <QLayout>
#include <qmailfolder.h>


SelectFolderDialog::SelectFolderDialog(const QMailFolderIdList &list, QWidget *parent)
    : QDialog( parent ),
      mFolderIds( list )
{
#ifdef QMAIL_QTOPIA
    QtopiaApplication::setMenuLike( this, true );
#endif
    setWindowTitle( tr( "Select folder" ) );

    mFolderList = new QListWidget( this );

    foreach (const QMailFolderId &folderId, mFolderIds) {
        QMailFolder folder(folderId);
        mFolderList->addItem(folder.path());
    }

    // Required for current item to be shown as selected(?)
    if (mFolderList->count())
        mFolderList->setCurrentRow( 0 );

    connect(mFolderList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(selected()) );

    QGridLayout *top = new QGridLayout( this );
    top->addWidget( mFolderList, 0, 0 );
}

SelectFolderDialog::~SelectFolderDialog()
{
}

QMailFolderId SelectFolderDialog::selectedFolderId() const
{
    return mFolderIds[mFolderList->currentRow()];
}

void SelectFolderDialog::selected()
{
    done(1);
}

