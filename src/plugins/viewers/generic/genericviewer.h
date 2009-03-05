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

#include <QContact>
#include <QObject>
#include <QString>

#include <qmailviewer.h>
#include <qmailviewerplugin.h>

#ifdef QTOPIA_HOMEUI
#include <private/homewidgets_p.h>
#endif

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

#ifdef QTOPIA_HOMEUI
    QString prettyName(QMailAddress address);
    QString recipients();
#endif

public slots:
    virtual bool setMessage(const QMailMessage& mail);
    virtual void setResource(const QUrl& name, QVariant var);
    virtual void clear();

    virtual void action(QAction* action);

    virtual void linkClicked(const QUrl& link);

protected slots:
    virtual void linkHighlighted(const QUrl& link);
#ifdef QTOPIA_HOMEUI
    virtual void replyActivated();
    virtual void senderActivated();
    virtual void recipientsActivated();
#endif

private:
    virtual void setPlainTextMode(bool plainTextMode);
    virtual void print() const;

    bool eventFilter(QObject* watched, QEvent* event);

    Browser* browser;
#ifdef QTOPIA_HOMEUI
    QWidget *mainWidget;
    ColumnSizer sizer;
    HomeContactButton *fromButton;
    HomeFieldButton *toButton;
    HomeActionButton *replyButton;
    HomeActionButton *deleteButton;
    HomeActionButton *backButton;
#endif
    QAction* plainTextModeAction;
    QAction* richTextModeAction;
    QAction* printAction;
    QAction* dialAction;
    QAction* messageAction;
    QAction* storeAction;
    QAction* contactAction;
    const QMailMessage* message;
    bool plainTextMode;
    bool containsNumbers;
    QContact contact;
};

#endif
