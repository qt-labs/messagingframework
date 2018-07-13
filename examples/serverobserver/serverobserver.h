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


#ifndef SERVER_OBSERVER_H
#define SERVER_OBSERVER_H

#include <QList>
#include <QMap>
#include <QWidget>
#include <QMainWindow>

#include <qmailserviceaction.h>

QT_BEGIN_NAMESPACE

class QLabel;
class QProgressBar;
class QPushButton;
class QLayout;

QT_END_NAMESPACE

class RowWidget;

class ServerObserver : public QMainWindow
{
    Q_OBJECT
public:
    ServerObserver(QWidget *parent = Q_NULLPTR);
    virtual ~ServerObserver();
private slots:
    void actionsChanged(QList<QSharedPointer< QMailActionInfo> > actions);
    void clear();
private:
    QLayout *_lay;
    QMailActionObserver *_actionObs;

    QMap<quint64, RowWidget *> _rows;
};

class RowWidget : public QWidget {
    Q_OBJECT
public:
    RowWidget(QSharedPointer<QMailActionInfo> action, QWidget *parent = Q_NULLPTR);

public slots:
    void progressChanged(uint x, uint y);
    void sendCancel();
private slots:
    void generateDescription();
private:
    static QString requestTypeToString(QMailServerRequestType t);

    QSharedPointer<QMailActionInfo> _action;
    QLabel *_description;
    QProgressBar *_progress;
    QPushButton *_cancel;
};



#endif
