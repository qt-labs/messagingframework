/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef GENERICVIEWER_H
#define GENERICVIEWER_H

#include <QObject>
#include <QString>
#include <qmailviewer.h>

class QAction;
class QMailMessage;
class QPushButton;
class QToolButton;
class Browser;

// A generic viewer able to show email, SMS or basic MMS
class GenericViewer : public QMailViewerInterface
{
    Q_OBJECT

public:
    GenericViewer(QWidget* parent = 0);
    virtual ~GenericViewer();

    virtual void scrollToAnchor(const QString& a);

    virtual QWidget *widget() const;

    virtual void addActions(QMenu* menu) const;

    virtual QString key() const;
    virtual QMailViewerFactory::PresentationType presentation() const;
    virtual QList<int> types() const;

public slots:
    virtual bool setMessage(const QMailMessage& mail);
    virtual void setResource(const QUrl& name, QVariant var);
    virtual void clear();

    virtual void triggered(bool);

    virtual void linkClicked(const QUrl& link);

protected slots:
    virtual void linkHighlighted(const QUrl& link);

private:
    virtual void setPlainTextMode(bool plainTextMode);

    bool eventFilter(QObject* watched, QEvent* event);

    Browser* browser;
    QAction* plainTextModeAction;
    QAction* richTextModeAction;
    const QMailMessage* message;
    bool plainTextMode;
    bool containsNumbers;
};

#endif
