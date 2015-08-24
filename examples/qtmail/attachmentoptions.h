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

#ifndef ATTACHMENTOPTIONS_H
#define ATTACHMENTOPTIONS_H

#include <qmailmessage.h>
#include <QByteArray>
#include <QDialog>
#include <QList>
#include <QSize>
#include <QString>

QT_BEGIN_NAMESPACE

class QByteArray;
class QLabel;
class QPushButton;
class QString;

QT_END_NAMESPACE

class AttachmentOptions : public QDialog
{
    Q_OBJECT

public:
    enum ContentClass
    {
        Text,
        Image,
        Media,
        Multipart,
        Other
    };

    AttachmentOptions(QWidget* parent);
    ~AttachmentOptions();

    QSize sizeHint() const;

signals:
    void retrieve(const QMailMessagePart& part);
    void retrievePortion(const QMailMessagePart& part, uint bytes);
    void respondToPart(const QMailMessagePart::Location& partLocation, QMailMessage::ResponseType);

public slots:
    void setAttachment(const QMailMessagePart& part);

    void viewAttachment();
    void saveAttachment();
    void retrieveAttachment();
    void forwardAttachment();

private:
    QSize _parentSize;
    QLabel* _name;
    QLabel* _type;
    //QLabel* _comment;
    QLabel* _sizeLabel;
    QLabel* _size;
    QPushButton* _view;
    QLabel* _viewer;
    QPushButton* _save;
    QLabel* _document;
    QPushButton* _retrieve;
    QPushButton* _forward;
    const QMailMessagePart* _part;
    ContentClass _class;
    QString _decodedText;
    QByteArray _decodedData;
    QStringList _temporaries;
};

#endif

