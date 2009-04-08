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
#include <QSet>
#include <QString>
#include <QTextBrowser>
#include <QUrl>
#include <QVariant>

class QWidget;
class QMailMessage;
class QMailMessagePart;
class QMailMessagePartContainer;

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

    static QString encodeUrlAndMail(const QString& txt);

public slots:
    virtual void setSource(const QUrl &name);

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    void displayPlainText(const QMailMessage* mail);
    void displayHtml(const QMailMessage* mail);

    void setTextResource(const QSet<QUrl>& names, const QString& textData);
    void setImageResource(const QSet<QUrl>& names, const QByteArray& imageData);
    void setPartResource(const QMailMessagePart& part);

    QString renderSimplePart(const QMailMessagePart& part);
    QString renderAttachment(const QMailMessagePart& part);
    QString renderPart(const QMailMessagePart& part);

    QString renderMultipart(const QMailMessagePartContainer& partContainer);

    QString describeMailSize(uint bytes) const;
    QString formatText(const QString& txt) const;
    QString smsBreakReplies(const QString& txt) const;
    QString noBreakReplies(const QString& txt) const;
    QString handleReplies(const QString& txt) const;
    QString buildParagraph(const QString& txt, const QString& prepend, bool preserveWs = false) const;

    static QString listRefMailTo(const QList<QMailAddress>& list);
    static QString refMailTo(const QMailAddress& address);
    static QString refNumber(const QString& number);
    static QString refUrl(const QString& url, const QString& scheme, const QString& trailing);

    QMap<QUrl, QVariant> resourceMap;
    QString (Browser::*replySplitter)(const QString&) const;

    mutable QList<QString> numbers;
};

#endif
