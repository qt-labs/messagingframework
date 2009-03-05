/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef ADDATTDIALOGPHONE_H
#define ADDATTDIALOGPHONE_H

#include <QAction>
#include <QDialog>

#include "addatt.h"

class QDocumentSelector;

// phone version, wraps AddAtt
class AddAttDialog : public QDialog, public AddAttBase
{
    Q_OBJECT
public:
    AddAttDialog(QWidget *parent = 0, QString name = QString(), Qt::WFlags f = 0);
    QList< AttachmentItem* > attachedFiles() const;
    void getFiles() {}
    void clear();
    QDocumentSelector* documentSelector() const;

public slots:
    void removeAttachment(AttachmentItem*);
    void removeCurrentAttachment();
    void attach( const QContent &doclnk, QMailMessage::AttachmentsAction );
    void selectAttachment();
    virtual void done(int r);

protected slots:
    void openFile(const QContent&);
    void updateDisplay(bool);

signals:
    void currentChanged(bool);
    void attachmentsChanged();

private:
    AddAtt *addAtt;
    QDocumentSelector *fileSelector;
    QDialog *fileSelectorDialog;
    QAction *removeAction;
};

#endif
