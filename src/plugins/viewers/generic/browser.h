/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef BROWSER_H
#define BROWSER_H

#include <QList>
#include <qmailaddress.h>
#include <QMap>
#include <QString>
#include <QTextBrowser>
#include <QUrl>
#include <QVariant>

class QWidget;
class QMailMessage;
class QMailMessagePart;

class Browser: public QTextBrowser
{
    Q_OBJECT

public:
    Browser(QWidget *parent = 0);
    virtual ~Browser();

    void setResource( const QUrl& name, QVariant var );
    void clearResources();

    void setMessage( const QMailMessage& mail, bool plainTextMode );

    void scrollBy(int dx, int dy);

    virtual QVariant loadResource(int type, const QUrl& name);

    QList<QString> embeddedNumbers() const;

public slots:
    virtual void setSource(const QUrl &name);

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    void displayPlainText(const QMailMessage* mail);
    void displayHtml(const QMailMessage* mail);

    void setTextResource(const QUrl& name, const QString& textData);
    void setImageResource(const QUrl& name, const QByteArray& imageData);
    void setPartResource(const QMailMessagePart& part);

    QString renderPart(const QMailMessagePart& part);
    QString renderAttachment(const QMailMessagePart& part);

    QString describeMailSize(uint bytes) const;
    QString formatText(const QString& txt) const;
    QString smsBreakReplies(const QString& txt) const;
    QString noBreakReplies(const QString& txt) const;
    QString handleReplies(const QString& txt) const;
    QString buildParagraph(const QString& txt, const QString& prepend, bool preserveWs = false) const;
    QString encodeUrlAndMail(const QString& txt) const;
    QString listRefMailTo(const QList<QMailAddress>& list) const;
    QString refMailTo(const QMailAddress& address) const;
    QString refNumber(const QString& number) const;

    QMap<QUrl, QVariant> resourceMap;
    QString (Browser::*replySplitter)(const QString&) const;

    mutable QList<QString> numbers;
};

#endif
