/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "observer.h"


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

Observer::Observer(QWidget *parent)
    : QMainWindow(parent),
      _actionObs(new QMailActionObserver(this))
{
    connect(_actionObs, SIGNAL(initialized()), this, SLOT(actionObserverInitialized()));
    connect(_actionObs, SIGNAL(actionStarted(QSharedPointer<QMailActionInfo>)), this, SLOT(addAction(QSharedPointer<QMailActionInfo>)));
    connect(_actionObs, SIGNAL(actionFinished(QMailActionId)), this, SLOT(removeAction(QMailActionId)));
    _actionObs->requestInitialization();

    QWidget *central = new QWidget(this);
    _lay = new QVBoxLayout();
    central->setLayout(_lay);

    this->setCentralWidget(central);
}

Observer::~Observer() {}

void Observer::actionObserverInitialized()
{
    foreach(QSharedPointer<QMailActionInfo> action, _actionObs->runningActions()) {
        addAction(action);
    }
}


void Observer::addAction(QSharedPointer<QMailActionInfo> action)
{
    Q_ASSERT(_actionObs->isInitialized());


    RowWidget *row = new RowWidget(action, this);
    _rows.insert(action->id(), row);
    _lay->addWidget(row);
}

void Observer::removeAction(QMailActionId action)
{

    RowWidget *r = _rows.value(action);
    _lay->removeWidget(r);
    delete r;
    _rows.remove(action);
}

RowWidget::RowWidget(QSharedPointer<QMailActionInfo> action, QWidget *parent)
    : QWidget(parent),
      _action(action),
      _description(new QLabel(QString("Action id: %1\nDescription: %2").arg(action->id()).arg(requestTypeToString(action->description())), this)),
      _progress(new QProgressBar(this)),
      _cancel(new QPushButton("cancel", this))
{
    QLayout *lay = new QHBoxLayout(this);
    lay->addWidget(_description);
    lay->addWidget(_progress);
    lay->addWidget(_cancel);

    connect(_action.data(), SIGNAL(progressChanged(uint,uint)), this, SLOT(progressChanged(uint,uint)));
    connect(_cancel, SIGNAL(clicked()), this, SLOT(sendCancel()));

    //qDe
}

QString RowWidget::requestTypeToString(QMailServerRequestType t)
{
    switch(t)
    {
    case AcknowledgeNewMessagesRequestType:
        return tr("Acknowledging new messages");
    case TransmitMessagesRequestType:
        return tr("Transmitting new messages");
    case RetrieveFolderListRequestType:
        return tr("Retrieving a list of folders");
    case RetrieveMessageListRequestType:
        return tr("Retrieving a list of message");
    case RetrieveMessagesRequestType:
        return tr("Retrieving messages");
    case RetrieveMessagePartRequestType:
        return tr("Retrieving part of a message");
    case RetrieveMessageRangeRequestType:
        return tr("Retrieving a range of messages");
    case RetrieveMessagePartRangeRequestType:
        return tr("Retrieving parts of a messages");
    case RetrieveAllRequestType:
        return tr("Retrieving everything");
    case ExportUpdatesRequestType:
        return tr("Exporting updates");
    case SynchronizeRequestType:
        return tr("Synchronizing");
    case CopyMessagesRequestType:
        return tr("Copying messages");
    case MoveMessagesRequestType:
        return tr("Moving messages");
    case FlagMessagesRequestType:
        return tr("Flagging messages");
    case CreateFolderRequestType:
        return tr("Creating a folder");
    case RenameFolderRequestType:
        return tr("Renaming a folder");
    case DeleteFolderRequestType:
        return tr("Deleting a folder");
    case CancelTransferRequestType:
        return tr("Canceling a transfer");
    case DeleteMessagesRequestType:
        return tr("Deleteing a message");
    case SearchMessagesRequestType:
        return tr("Searching");
    case CancelSearchRequestType:
        return tr("Cancelling search");
    case ListActionsRequestType:
        return tr("Listing actions");
    case ProtocolRequestRequestType:
        return tr("Direct protocol request");
        // No default, to get warning when requests added
    }

    qWarning() << "Did not handle:" << t;
    Q_ASSERT(false);
    return tr("Unknown/handled request.");
}

void RowWidget::sendCancel()
{
    _action->cancelOperation();
}


void RowWidget::progressChanged(uint x, uint y) {
    _progress->setMaximum(y);
    _progress->setValue(x);
}
