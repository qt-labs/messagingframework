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

#ifndef ATTACHMENTLISTWIDGET_H
#define ATTACHMENTLISTWIDGET_H

#include <QWidget>

class AttachmentListView;
class AttachmentListModel;
class AttachmentListDelegate;

QT_BEGIN_NAMESPACE

class QListView;
class QStringListModel;
class QLabel;
class QTreeView;
class QModelIndex;

QT_END_NAMESPACE

class AttachmentListWidget : public QWidget
{
    Q_OBJECT

public:
    AttachmentListWidget(QWidget* parent = Q_NULLPTR);
    QStringList attachments() const;
    QString attachmentAt(int index) const;
    int count() const;
    bool isEmpty() const;

public slots:
    void clear();
    void addAttachment(const QString& attachment);
    void addAttachments(const QStringList& attachments);
    void removeAttachment(const QString& attachment);

signals:
    void attachmentsAdded(const QStringList& attachments);
    void attachmentsRemoved(const QString& attachment);

private slots:
    void clearClicked();
    void removeAttachmentAtIndex(int);

private:
    AttachmentListView* m_listView;
    AttachmentListModel* m_model;
    AttachmentListDelegate* m_delegate;
    QStringList m_attachments;
    QLabel* m_clearLink;
};

#endif
