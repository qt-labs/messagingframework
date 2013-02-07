/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SYMBIANDIR_H
#define SYMBIANDIR_H

#include "qmfdatastorage.h"
#include <QString>
#include <QStringList>

class SymbianQMFDataStorage;

class SymbianDir
{
public:
    enum Filter {
        NoFilter    = -1,
        Dirs        = 0x001,
        Files       = 0x002,
        Drives      = 0x004,
        AllEntries  = Dirs | Files | Drives,
        CaseSensitive = 0x800,
        NoDotAndDotDot = 0x1000
    };
    Q_DECLARE_FLAGS(Filters, Filter)

    SymbianDir(const SymbianDir &);
    SymbianDir(const QString &path = QString());
    ~SymbianDir();

    void setPath(const QString &path);
    QString absolutePath() const;
    QString filePath(const QString &fileName) const;

    void setNameFilters(const QStringList &nameFilters);
    QStringList entryList(int filters = NoFilter) const;

    bool rmdir(const QString &dirName) const;
    bool mkpath(const QString &dirPath) const;
    bool rmpath(const QString &dirPath) const;
    bool remove(const QString &fileName);

    bool exists() const;

    static inline SymbianDir root() { return SymbianDir(rootPath()); }
    static QString rootPath();

private: // Data
    mutable SymbianQMFDataStorage m_dataStorage;
    QString m_path;
    QStringList m_nameFilters;
};

#endif // SYMBIANDIR_H

// End of File
