/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef SELECTCOMPOSERWIDGET_H
#define SELECTCOMPOSERWIDGET_H

#include <QPair>
#include <qmailmessage.h>
#include <QString>
#include <QWidget>

class QListWidget;
class QListWidgetItem;
class SelectListWidget;

class SelectComposerWidget : public QWidget
{
    Q_OBJECT

public:
    SelectComposerWidget( QWidget* parent );

    void setSelected(const QString& selected, QMailMessage::MessageType type);

    QString singularKey() const;

    QList<QMailMessage::MessageType> availableTypes() const;

    QPair<QString, QMailMessage::MessageType> currentSelection() const;

signals:
    void selected(const QPair<QString, QMailMessage::MessageType> &);
    void cancel();

protected slots:
    void accept(QListWidgetItem* item);
    void refresh();

protected:
    void keyPressEvent(QKeyEvent *e);
    void init();

private:
    SelectListWidget* m_listWidget;
};

#endif
