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
    GenericViewer(QWidget* parent = 0);

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
