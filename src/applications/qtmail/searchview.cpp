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

#include "searchview.h"
#include <QMenu>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QDateTimeEdit>
#include <QComboBox>
#include <qmailfolder.h>
#include <qmailmessage.h>
#include <QApplication>

SearchView::SearchView(QWidget* parent)
    : QDialog(parent)
{
    setupUi( this );

    delete layout();
    QGridLayout *g = new QGridLayout(this);
    sv = new QScrollArea(this);
    sv->setFocusPolicy(Qt::NoFocus);
    sv->setWidgetResizable( true );
    sv->setFrameStyle(QFrame::NoFrame);
    sv->setWidget(searchFrame);
    sv->setFocusPolicy(Qt::NoFocus);
    g->addWidget(sv, 0, 0);
    int dw = QApplication::desktop()->availableGeometry().width();
    searchFrame->setMaximumWidth(dw - qApp->style()->pixelMetric(QStyle::PM_SliderLength) + 4 );
    sv->setMaximumWidth(dw);
    setWindowTitle( tr("Search") );
    QIcon abicon(":icon/addressbook/AddressBook");
    pickAddressAction = new QAction(abicon,
                                    tr("From contacts",
                                       "Find email address "
                                       "from Contacts application"),
                                    this);
    connect(pickAddressAction, SIGNAL(triggered()),
            this, SLOT(pickAddressSlot()));

    init();
}

SearchView::~SearchView()
{
}

void SearchView::init()
{
    dateAfter = QDate::currentDate();
    dateAfterButton->setDate( dateAfter );
    dateBefore = QDate::currentDate();
    dateBeforeButton->setDate( dateBefore );
    setTabOrder(mailbox, status);
    setTabOrder(status, fromLine);
    setTabOrder(fromLine, toLine);
    setTabOrder(toLine,subjectLine);
    setTabOrder(subjectLine,bodyLine);
    setTabOrder(dateAfterGroupBox, dateAfterButton);
    setTabOrder(dateAfterButton, dateBeforeGroupBox);
    setTabOrder(dateBeforeGroupBox, dateBeforeButton);
}

QMailMessageKey SearchView::searchKey() const
{
    QMailMessageKey resultKey;

    static QMailMessageKey inboxFolderKey(QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::InboxFolder)));
    static QMailMessageKey outboxFolderKey(QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::OutboxFolder)));
    static QMailMessageKey draftsFolderKey(QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::DraftsFolder)));
    static QMailMessageKey sentFolderKey(QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::SentFolder)));
    static QMailMessageKey trashFolderKey(QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::TrashFolder)));
    static QMailMessageKey inboxWithImapKey(inboxFolderKey | ~(outboxFolderKey | draftsFolderKey | sentFolderKey | trashFolderKey));

    //folder

    switch(mailbox->currentIndex())
    {
        case 1: //inbox with imap subfolders
            resultKey &= inboxWithImapKey;
            break;
        case 2: //outbox
            resultKey &= outboxFolderKey;
            break;
        case 3: //drafts
            resultKey &= draftsFolderKey;
            break;
        case 4: //sent
            resultKey &= sentFolderKey;
            break;
        case 5: //trash
            resultKey &= trashFolderKey;
            break;
    }

    //status

    switch(status->currentIndex())
    {
         case 1: //read
            resultKey &= (QMailMessageKey::status(QMailMessage::Read,QMailDataComparator::Includes) |
                          QMailMessageKey::status(QMailMessage::ReadElsewhere,QMailDataComparator::Includes));
            break;
        case 2: //unread
            resultKey &= (~QMailMessageKey::status(QMailMessage::Read,QMailDataComparator::Includes) &
                          ~QMailMessageKey::status(QMailMessage::ReadElsewhere,QMailDataComparator::Includes));
            break;
        case 3: //replied
            resultKey &= ~QMailMessageKey::status(QMailMessage::Replied,QMailDataComparator::Includes);
            break;
        case 4: //replied
            resultKey &= QMailMessageKey::status(QMailMessage::Removed,QMailDataComparator::Includes);
            break;
    }

    if(!fromLine->text().isEmpty())
        resultKey &= QMailMessageKey::sender(fromLine->text(),QMailDataComparator::Includes);

    if(!toLine->text().isEmpty())
        resultKey &= QMailMessageKey::recipients(toLine->text(),QMailDataComparator::Includes);

    if(!subjectLine->text().isEmpty())
        resultKey &= QMailMessageKey::subject(subjectLine->text(),QMailDataComparator::Includes);

    //dates

    if(dateAfterGroupBox->isChecked())
    {
        QDateTime afterDate(dateAfterButton->date());
        resultKey &= QMailMessageKey::timeStamp(afterDate,QMailDataComparator::GreaterThan);
    }

    if(dateBeforeGroupBox->isChecked())
    {
        QDateTime beforeDate(dateBeforeButton->date());
        resultKey &= QMailMessageKey::timeStamp(beforeDate,QMailDataComparator::LessThan);
    }

    return resultKey;
}

QString SearchView::bodyText() const
{
    return bodyLine->text();
}

void SearchView::reset()
{
    mailbox->setCurrentIndex(0);
    status->setCurrentIndex(0);
    fromLine->clear();
    toLine->clear();
    subjectLine->clear();
    bodyLine->clear();
    dateAfterGroupBox->setChecked(false);
    dateBeforeGroupBox->setChecked(false);

    mailbox->setFocus();
}

void SearchView::updateActions()
{
    pickAddressAction->setVisible(fromLine->hasFocus() || toLine->hasFocus());
}

void SearchView::pickAddressSlot()
{
    QString txt;
    QLineEdit *edit = fromLine;
    if (toLine->hasFocus())
        edit = toLine;
}
