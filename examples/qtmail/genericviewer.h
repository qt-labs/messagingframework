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

#ifndef GENERICVIEWER_H
#define GENERICVIEWER_H

#include <QObject>
#include <QString>
#include <qmailviewer.h>

class AttachmentOptions;
class BrowserWidget;
class QMailMessage;

QT_BEGIN_NAMESPACE

class QAction;
class QPushButton;
class QToolButton;

QT_END_NAMESPACE

// A generic viewer able to show email, SMS or basic MMS
class GenericViewer : public QMailViewerInterface
{
    Q_OBJECT

public:
    GenericViewer(QWidget* parent = Q_NULLPTR);

    QWidget* widget() const;

    void addActions(const QList<QAction*>& actions);
    void removeAction(QAction* action);

    virtual void scrollToAnchor(const QString& a);
    virtual QString key() const;
    virtual QMailViewerFactory::PresentationType presentation() const;
    virtual QList<QMailMessage::ContentType> types() const;

public slots:
    virtual bool setMessage(const QMailMessage& mail);
    virtual void setResource(const QUrl& name, QVariant var);
    virtual void clear();
    virtual void triggered(bool);
    virtual void linkClicked(const QUrl& link);

protected slots:
    virtual void dialogFinished(int);

private:
    virtual void setPlainTextMode(bool plainTextMode);
    bool eventFilter(QObject* watched, QEvent* event);

private:
    BrowserWidget* browser;
    QAction* plainTextModeAction;
    QAction* richTextModeAction;
    AttachmentOptions* attachmentDialog;
    const QMailMessage* message;
    bool plainTextMode;
    bool containsNumbers;
};

#endif
