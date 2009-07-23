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

#include "quicksearchwidget.h"
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <qmailmessage.h>

QuickSearchWidget::QuickSearchWidget(QWidget* parent)
:
QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    m_clearButton = new QToolButton(this);
    m_clearButton->setIcon(QIcon(":icon/clear_right"));
    connect(m_clearButton,SIGNAL(clicked()),this,SLOT(reset()));
    layout->addWidget(m_clearButton);

    layout->addWidget(new QLabel("Search:"));
    m_searchTerms = new QLineEdit(this);
    connect(m_searchTerms,SIGNAL(textChanged(QString)),this,SLOT(searchTermsChanged()));
    layout->addWidget(m_searchTerms);

    layout->addWidget(new QLabel("Status:"));

    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem(QIcon(":icon/exec"),"Any Status",QMailMessageKey());
    m_statusCombo->addItem(QIcon(":icon/mail_generic"),"Unread",QMailMessageKey::status(QMailMessage::Read,QMailDataComparator::Excludes));
    m_statusCombo->addItem(QIcon(":icon/new"),"New",QMailMessageKey::status(QMailMessage::New));
    m_statusCombo->addItem(QIcon(":icon/mail_reply"),"Replied",QMailMessageKey::status(QMailMessage::Replied));
    m_statusCombo->addItem(QIcon(":icon/mail_forward"),"Forwarded",QMailMessageKey::status(QMailMessage::Forwarded));
    m_statusCombo->addItem(QIcon(":/icon/attach"),"Has Attachment",QMailMessageKey::status(QMailMessage::HasAttachments));
    connect(m_statusCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(searchTermsChanged()));
    layout->addWidget(m_statusCombo);

    m_fullSearchButton = new QToolButton(this);
    m_fullSearchButton->setIcon(QIcon(":icon/find"));
    connect(m_fullSearchButton,SIGNAL(clicked(bool)),this,SIGNAL(fullSearchRequested()));
    layout->addWidget(m_fullSearchButton);
}

QMailMessageKey QuickSearchWidget::searchKey() const
{
    return buildSearchKey();
}

void QuickSearchWidget::reset(bool emitReset)
{
    blockSignals(!emitReset);
    m_statusCombo->setCurrentIndex(0);
    m_searchTerms->clear();
    blockSignals(false);
}

void QuickSearchWidget::searchTermsChanged()
{
    QMailMessageKey key = buildSearchKey();
    emit quickSearchRequested(key);
}

QMailMessageKey QuickSearchWidget::buildSearchKey() const
{
    if(m_searchTerms->text().isEmpty() && m_statusCombo->currentIndex() == 0)
        return QMailMessageKey();
    QMailMessageKey subjectKey = QMailMessageKey::subject(m_searchTerms->text(),QMailDataComparator::Includes);
    QMailMessageKey senderKey = QMailMessageKey::sender(m_searchTerms->text(),QMailDataComparator::Includes);

    QMailMessageKey statusKey = qvariant_cast<QMailMessageKey>(m_statusCombo->itemData(m_statusCombo->currentIndex()));
    if(!statusKey.isEmpty())
        return ((subjectKey | senderKey) & statusKey);
    return subjectKey | senderKey;
}

