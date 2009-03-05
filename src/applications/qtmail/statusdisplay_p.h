/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef STATUSDISPLAY_H
#define STATUSDISPLAY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QStatusBar>

class QProgressBar;

class StatusDisplay : public QStatusBar
{
    Q_OBJECT

public:
    StatusDisplay(QWidget* parent = 0);

public slots:
    void showStatus(bool visible);
    void displayStatus(const QString& txt);
    void displayProgress(uint value, uint range);
    void clearStatus();

private:
    bool suppressed;
    QProgressBar* m_progressBar;
};

#endif
