/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef ATTACHMENTLISTWIDGET_H
#define ATTACHMENTLISTWIDGET_H

#include <QWidget>

class QListView;
class QStringListModel;
class QLabel;
class QTreeView;
class AttachmentListView;
class AttachmentListModel;
class AttachmentListDelegate;
class QModelIndex;

class AttachmentListWidget : public QWidget
{
    Q_OBJECT

public:
    AttachmentListWidget(QWidget* parent = 0);
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
