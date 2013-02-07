/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
    int maximumBufferSize() const;

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
