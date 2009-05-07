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

public slots:
    void clear();
    void addAttachment(const QString& attachment);
    void addAttachments(const QStringList& attachments);
    void removeAttachment(const QString& attachment);

signals:
    void attachmentsAdded(const QStringList& attachments);
    void attachmentsRemoved(const QString& attachment);

private slots:
    void updateClearLabel();
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
