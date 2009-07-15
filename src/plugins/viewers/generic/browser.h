/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BROWSERWIDGET_H
#define BROWSERWIDGET_H

#include <QList>
#include <qmailaddress.h>
#include <QMap>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QWidget>

class QMailMessage;
class QMailMessagePart;
class QMailMessagePartContainer;
#ifdef USING_WEBKIT
class QWebView;
#else
class QTextBrowser;
#endif

class BrowserWidget : public QWidget
{
    Q_OBJECT

public:
    BrowserWidget(QWidget *parent = 0);

    void setResource( const QUrl& name, QVariant var );
    void clearResources();

    void setMessage( const QMailMessage& mail, bool plainTextMode );

    virtual QVariant loadResource(int type, const QUrl& name);

    QList<QString> embeddedNumbers() const;

    static QString encodeUrlAndMail(const QString& txt);

    void scrollToAnchor(const QString& anchor);
    void setPlainText(const QString& text);

signals:
    void anchorClicked(const QUrl&);
    void highlighted(const QUrl&);

public slots:
    virtual void setSource(const QUrl &name);

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
    static QString refUrl(const QString& url, const QString& scheme, const QString& leading, const QString& trailing);

private:
    QMap<QUrl, QVariant> resourceMap;
    QString (BrowserWidget::*replySplitter)(const QString&) const;
    mutable QList<QString> numbers;
#ifdef USING_WEBKIT
    QWebView* m_webView;
#else
    QTextBrowser* m_textBrowser;
#endif
};

#endif
