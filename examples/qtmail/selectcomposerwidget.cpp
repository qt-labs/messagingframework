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

#include "selectcomposerwidget.h"
#include "qmailcomposer.h"
#include <qmaillog.h>
#include <QKeyEvent>
#include <qmailstore.h>
#include <qmailaccount.h>
#include <QListWidget>
#include <QBoxLayout>
#include <QSet>

class SelectListWidget : public QListWidget
{
    Q_OBJECT

public:
    SelectListWidget(QWidget* parent = Q_NULLPTR);

signals:
    void cancel();

protected:
    void keyPressEvent(QKeyEvent* e);

};

SelectListWidget::SelectListWidget(QWidget* parent)
:
QListWidget(parent)
{
}

void SelectListWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Back) {
        e->accept();
        // Allow this event to be acted upon before we return control to our parent
        emit cancel();
        return;
    }
    QListWidget::keyPressEvent( e );
}

class SelectListWidgetItem : public QListWidgetItem
{
public:
    SelectListWidgetItem(const QString& id, QMailMessage::MessageType type, QListWidget* listWidget)
        : QListWidgetItem(listWidget),
          _key(id),
          _type(type)
    {
        setText(QMailComposerFactory::name(_key, _type));
        setIcon(QMailComposerFactory::displayIcon(_key, _type));
    }

    const QString& key() const { return _key; }
    QMailMessage::MessageType type() { return _type; }

private:
    QString _key;
    QMailMessage::MessageType _type;
};

SelectComposerWidget::SelectComposerWidget( QWidget *parent )
    :
    QWidget( parent ),
    m_listWidget(0)
{
    init();
}

void SelectComposerWidget::init()
{
    m_listWidget = new SelectListWidget(this);
    m_listWidget->setFrameStyle( QFrame::NoFrame );
    connect(m_listWidget, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(accept(QListWidgetItem*)));
    connect(m_listWidget, SIGNAL(cancel()),this,SIGNAL(cancel()));
    QWidget::setFocusProxy(m_listWidget);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_listWidget);
    QWidget::setLayout(layout);

    if (QMailStore *store = QMailStore::instance()) {
        connect(store, SIGNAL(accountsAdded(QMailAccountIdList)), this, SLOT(refresh()));
        connect(store, SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(refresh()));
        connect(store, SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(refresh()));
    }

    refresh();
}

QList<QMailMessage::MessageType> SelectComposerWidget::availableTypes() const
{
    QList<QMailMessage::MessageType> results;

    for (int i = 0; i < m_listWidget->count(); ++i) {
        SelectListWidgetItem* item = static_cast<SelectListWidgetItem*>(m_listWidget->item(i));
        results.append(item->type());
    }

    return results;
}

QPair<QString, QMailMessage::MessageType> SelectComposerWidget::currentSelection() const
{
    for (int i = 0; i < m_listWidget->count(); ++i) {
        SelectListWidgetItem* item = static_cast<SelectListWidgetItem*>(m_listWidget->item(i));
        if (item->isSelected())
            return qMakePair(item->key(), item->type());
    }

    return qMakePair(QString(), QMailMessage::AnyType);
}


void SelectComposerWidget::setSelected(const QString& key, QMailMessage::MessageType type)
{
    int selectIndex = 0;

    if (!key.isEmpty()) {
        for (int i = 1; i < m_listWidget->count() && selectIndex == 0; ++i) {
            SelectListWidgetItem* item = static_cast<SelectListWidgetItem*>(m_listWidget->item(i));
            if (item->key() == key && (item->type() == type || type == QMailMessage::AnyType)) {
                selectIndex = i;
                break;
            }
        }
    }

    m_listWidget->setCurrentRow(selectIndex);
}

QString SelectComposerWidget::singularKey() const
{
    if (m_listWidget->count() == 1)
        return static_cast<SelectListWidgetItem*>(m_listWidget->item(0))->key();

    return QString();
}

void SelectComposerWidget::accept(QListWidgetItem* listItem)
{
    if (listItem) {
        SelectListWidgetItem* item = static_cast<SelectListWidgetItem*>(listItem);

        // Allow this event to be acted upon before we return control to our parent
        emit selected(qMakePair(item->key(), item->type()));
    }
}

void SelectComposerWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Back) {
        e->accept();
        // Allow this event to be acted upon before we return control to our parent
        emit cancel();
        return;
    }

    QWidget::keyPressEvent( e );
}

static int messageTypeValue(QMailMessage::MessageType type)
{
    // List message types in ascending screen order
    if (type == QMailMessage::Sms) return 0;
    if (type == QMailMessage::Instant) return 1;
    if (type == QMailMessage::Mms) return 2;
    if (type == QMailMessage::Email) return 3;
    if (type == QMailMessage::System) return 4;
    return 5;
}

typedef QPair<QString, QMailMessage::MessageType> OptionType;

static bool compareOptionsByType(const OptionType &lhs, const OptionType &rhs)
{
    if (lhs.second != rhs.second)
        return (messageTypeValue(lhs.second) < messageTypeValue(rhs.second));

    return (lhs.first < rhs.first);
}

void SelectComposerWidget::refresh()
{
    // Find what types of outgoing messages our accounts support
    QSet<QMailMessage::MessageType> _sendTypes;

    QMailAccountKey transmitKey(QMailAccountKey::status(QMailAccount::CanTransmit, QMailDataComparator::Includes));
    QMailAccountIdList accountIds = QMailStore::instance()->queryAccounts(transmitKey);
    foreach(const QMailAccountId &id, accountIds) {
        QMailAccount account(id);
        foreach (QMailMessage::MessageType type, QMailMessageTypeList() << QMailMessage::Sms
                                                                        << QMailMessage::Instant
                                                                        << QMailMessage::Mms
                                                                        << QMailMessage::Email) {
            if (account.messageType() & type) {
                _sendTypes.insert(type);
            }
        }
    }

    m_listWidget->clear();

    QList<OptionType> options;

    foreach (const QString& key, QMailComposerFactory::keys())
        foreach (QMailMessage::MessageType type, QMailComposerFactory::messageTypes(key))
            if (_sendTypes.contains(type))
                options.append(qMakePair(key, type));

    std::sort(options.begin(), options.end(), compareOptionsByType);

    foreach (const OptionType &option, options)
        (void)new SelectListWidgetItem(option.first, option.second, m_listWidget);
}

#include <selectcomposerwidget.moc>

