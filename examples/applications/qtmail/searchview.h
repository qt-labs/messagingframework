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

#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include <QMainWindow>
#include <qmailmessagekey.h>
#include <qmailserviceaction.h>

class QPushButton;
class QRadioButton;
class QCheckBox;
class QLineEdit;
class QToolButton;
class MessageListView;
class SearchTermsComposer;
class FolderSelectorWidget;
class QMailSearchAction;
class QStatusBar;
class SearchButton;
class BodySearchWidget;

class SearchView : public QMainWindow
{
    Q_OBJECT

public:
    SearchView(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~SearchView();
    void setVisible(bool visible);

signals:
    void searchResultSelected(const QMailMessageId& id);

public slots:
    void reset();
    void close();

private:
    void setupUi();
    QMailMessageKey searchKey() const;

private slots:
    void startSearch();
    void stopSearch();
    void messageIdsMatched(const QMailMessageIdList& ids);
    void searchProgressChanged(uint value, uint total);
    void searchActivityChanged(QMailServiceAction::Activity a);

private:
    SearchButton* m_searchButton;
    QPushButton* m_resetButton;
    QPushButton* m_closeButton;
    FolderSelectorWidget* m_folderSelectorWidget;
    SearchTermsComposer* m_searchTermsComposer;
    BodySearchWidget* m_bodySearchWidget;
    MessageListView* m_searchResults;
    QMailSearchAction* m_searchAction;
    QStatusBar* m_statusBar;
};

#endif
