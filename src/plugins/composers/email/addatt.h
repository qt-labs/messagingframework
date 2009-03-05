/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/
#ifndef ADDATT_H
#define ADDATT_H

#include <qcontent.h>
#include <qtopiaglobal.h>

#include <QListWidget>
#include <QPushButton>
#include <QMailMessage>
#include <QMenu>
#include <QString>
#include <QList>

class QMailMessage;

class AttachmentItem : public QListWidgetItem
{
public:
    AttachmentItem(QListWidget *parent, const QContent&, QMailMessage::AttachmentsAction);
    ~AttachmentItem();

    const QContent& document() const;
    int sizeKB() const;
    QMailMessage::AttachmentsAction action() const;

private:
    QContent mAttachment;
    int mSizeKB;
    QMailMessage::AttachmentsAction mAction;
};

class AddAttBase
{
public:
    virtual ~AddAttBase();
    QList< AttachmentItem* > attachedFiles() const;
    void clear();
    void setHighlighted();
    virtual void getFiles() = 0;
    bool addAttachment(const QContent&, QMailMessage::AttachmentsAction action);
    int totalSizeKB();

protected:
    QStringList mimeTypes();
    QListWidget *attView;
    bool modified;
    QListWidgetItem* addItem;
};

class AddAtt : public QWidget, public AddAttBase
{
    Q_OBJECT
public:
    AddAtt(QWidget *parent = 0, const char *name = 0, Qt::WFlags f = 0);
    void getFiles() {}

public slots:
    void removeCurrentAttachment();

signals:
    void currentChanged(bool);
    void selected(AttachmentItem*);
    void addNewAttachment();

private slots:
    void currentItemChanged(QListWidgetItem*,QListWidgetItem* previous);
    void itemActivated(QListWidgetItem*);
};

#include "addattdialogphone.h"

#endif
