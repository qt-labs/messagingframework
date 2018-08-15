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

#include "selectfolder.h"
#include "emailfolderview.h"
#include "folderdelegate.h"
#include "emailfoldermodel.h"
#include <qmailfolder.h>
#include <qmailmessageset.h>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>

SelectFolderDialog::SelectFolderDialog(FolderModel *model, QWidget *parent)
    : QDialog( parent )
{
    setWindowTitle( tr( "Select folder" ) );

    FolderDelegate* del = new FolderDelegate(this);
    del->setShowStatus(false);

    EmailFolderView *emailView = new EmailFolderView(this);
    if (EmailFolderModel *emailModel = qobject_cast<EmailFolderModel*>(model)) {
        emailView->setModel(emailModel);
    } else {
        qWarning() << "model must must be a non-abstract subclass of FolderModel";
    }

    m_folderView = emailView;
    m_folderView->setItemDelegate(del);
    m_folderView->expandAll();

    connect(m_folderView, SIGNAL(selected(QMailMessageSet*)), this, SLOT(selected(QMailMessageSet*)));

    QGridLayout *top = new QGridLayout( this );
    top->addWidget(m_folderView);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;

    buttonsLayout->addStretch();

    m_okButton = new QPushButton(QLatin1String("Ok"), this);
    buttonsLayout->addWidget(m_okButton);
    connect(m_okButton,SIGNAL(clicked(bool)),this,SLOT(accept()));

    QPushButton* cancelButton = new QPushButton(QLatin1String("Cancel"), this);
    buttonsLayout->addWidget(cancelButton);
    connect(cancelButton,SIGNAL(clicked(bool)),this,SLOT(reject()));

    top->addLayout(buttonsLayout,1,0);
}

void SelectFolderDialog::setInvalidSelections(const QList<QMailMessageSet*> &invalidSelections)
{
    m_invalidSelections = invalidSelections;
    m_okButton->setEnabled(!m_invalidSelections.contains(m_folderView->currentItem()));
}

void SelectFolderDialog::selected(QMailMessageSet *item)
{
    m_okButton->setEnabled(!m_invalidSelections.contains(item));
}

QMailMessageSet* SelectFolderDialog::selectedItem() const
{
    return m_folderView->currentItem();
}

