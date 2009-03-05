/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGEVIEWER_H
#define MESSAGEVIEWER_H
#include "ui_messageviewerbase.h"

class QMailMessageId;

class MessageViewer : public QWidget, public Ui_MessageViewerBase
{
    Q_OBJECT
public:
    MessageViewer( QWidget *parent = 0, Qt::WFlags f = 0 );
    ~MessageViewer();

private slots:
    void showMessageList();
    void viewMessage(const QMailMessageId&);
    void showContactList();

private:
    QWidget* contactSelector;
    QWidget* messageSelector;
};

#endif
