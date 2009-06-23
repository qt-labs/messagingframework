/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
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

#include "selectfolder.h"
#include <QListWidget>
#include <QLayout>
#include <qmailfolder.h>
#include "emailfolderview.h"
#include <qmailmessageset.h>
#include "folderdelegate.h"
#include "emailfoldermodel.h"
#include <QPushButton>

SelectFolderDialog::SelectFolderDialog(const QMailFolderIdList &list, QWidget *parent)
    : QDialog( parent )
{
    setWindowTitle( tr( "Select folder" ) );

    m_folderList = new EmailFolderView(this);
    m_model = new EmailFolderModel(list,this);

    m_folderList->setModel(m_model);
    FolderDelegate* del = new FolderDelegate(this);
    del->setShowStatus(false);
    m_folderList->setItemDelegate(del);
    m_folderList->expandAll();

    QGridLayout *top = new QGridLayout( this );
    top->addWidget(m_folderList);

    QHBoxLayout* buttonsLayout = new QHBoxLayout(this);

    buttonsLayout->addStretch();

    QPushButton* okButton = new QPushButton("Ok",this);
    buttonsLayout->addWidget(okButton);
    connect(okButton,SIGNAL(clicked(bool)),this,SLOT(accept()));

    QPushButton* cancelButton = new QPushButton("Cancel",this);
    buttonsLayout->addWidget(cancelButton);
    connect(cancelButton,SIGNAL(clicked(bool)),this,SLOT(reject()));


    top->addLayout(buttonsLayout,1,0);

}

QMailFolderId SelectFolderDialog::selectedFolderId() const
{
    return m_folderList->currentFolderId();
}

QMailAccountId SelectFolderDialog::selectedAccountId() const
{
    return m_folderList->currentAccountId();
}
