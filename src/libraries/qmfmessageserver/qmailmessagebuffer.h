/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGEBUFFER_H
#define QMAILMESSAGEBUFFER_H

#include <QObject>
#include <QList>
#include <QTime>
#include <QVariant>

class QMailMessage;
class QMailMessageBufferPrivate;

#include <qmailglobal.h>


class MESSAGESERVER_EXPORT QMailMessageBufferFlushCallback
{
public:
    virtual ~QMailMessageBufferFlushCallback() {}
    virtual void messageFlushed(QMailMessage *message) = 0;
};

class MESSAGESERVER_EXPORT QMailMessageBuffer : public QObject
{
    Q_OBJECT
public:
    QMailMessageBuffer(QObject *parent = 0);
    virtual ~QMailMessageBuffer();

    static QMailMessageBuffer *instance();

    bool addMessage(QMailMessage *message);
    bool updateMessage(QMailMessage *message);
    bool setCallback(QMailMessage *message, QMailMessageBufferFlushCallback *callback);

    void flush();

    void removeCallback(QMailMessageBufferFlushCallback *callback);

signals:
    void flushed();

private slots:
    void messageTimeout();
    void readConfig();

private:
    friend class QMailMessageBufferPrivate;
    struct BufferItem
    {
        BufferItem(bool _add, QMailMessageBufferFlushCallback *_callback, QMailMessage *_message)
            : add(_add)
            , callback(_callback)
            , message(_message)
        {}

        bool add;
        QMailMessageBufferFlushCallback *callback;
        QMailMessage *message;
    };

    void messageFlush();
    int messagePending();
    bool isFull();
    BufferItem *get_item(QMailMessage *message);

    QScopedPointer<QMailMessageBufferPrivate> d;
};

#endif
