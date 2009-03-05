/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
#ifdef QMAIL_QTOPIA
#include <QContactSelector>
#include <QSoftMenuBar>
#include <QtopiaApplication>
#else
#include <QApplication>
#endif

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
#ifdef QMAIL_QTOPIA
    QMenu *searchContext = QSoftMenuBar::menuFor(this);
    connect(searchContext, SIGNAL(aboutToShow()), this, SLOT(updateActions()));
#endif
    QIcon abicon(":icon/addressbook/AddressBook");
    pickAddressAction = new QAction(abicon,
                                    tr("From contacts",
                                       "Find email address "
                                       "from Contacts application"),
                                    this);
#ifdef QMAIL_QTOPIA
    searchContext->addAction(pickAddressAction);
    QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Finish);
#endif
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
#ifdef QMAIL_QTOPIA
    QContactSelector selector;
    selector.setObjectName("select-contact");

    QContactModel model(&selector);

    QSettings config( "Trolltech", "Contacts" );
    config.beginGroup( "default" );
    if (config.contains("SelectedSources/size")) {
        int count = config.beginReadArray("SelectedSources");
        QSet<QPimSource> set;
        for(int i = 0; i < count; ++i) {
            config.setArrayIndex(i);
            QPimSource s;
            s.context = QUuid(config.value("context").toString());
            s.identity = config.value("identity").toString();
            set.insert(s);
        }
        config.endArray();
        model.setVisibleSources(set);
    }

    selector.setModel(&model);
    selector.setAcceptTextEnabled(false);

    if (QtopiaApplication::execDialog(&selector) == QDialog::Accepted) {
        QContact contact(selector.selectedContact());
        edit->setText(contact.defaultEmail());
    }
#endif
}
