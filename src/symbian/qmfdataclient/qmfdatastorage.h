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

#ifndef SYMBIANQMFDATASTORAGE_H
#define SYMBIANQMFDATASTORAGE_H

#include "qmfdatasession.h"
#include <QSharedDataPointer>
#include <QEventLoop>

class QString;
class SymbianQMFDataStoragePrivate;

class SymbianQMFDataStorage
{
public:
    SymbianQMFDataStorage();
    SymbianQMFDataStorage(const SymbianQMFDataStorage& other);
    ~SymbianQMFDataStorage();

    SymbianQMFDataStorage& operator=(const SymbianQMFDataStorage& other);

    bool connect();
    bool createDatabase(QString name);
    bool openProtectedFile(RFile& file, QString filePath);
    bool openOrCreateProtectedFile(RFile& file, QString filePath);
    bool protectedFileExists(QString filePath);
    bool removeProtectedFile(QString filePath);
    bool renameProtectedFile(QString oldFilePath, QString newFilePath);
    bool protectedDirectoryExists(QString path);
    bool makeProtectedDirectory(QString path);
    bool removeProtectedDirectory(QString path);
    bool removeProtectedPath(QString path);
    QStringList listDirectoryEntries(QString path);

private:
    void convertQtFilePathToSymbianPath(QString& filePath);
    void convertQtPathToSymbianPath(QString& path);

private: // Data
    SymbianQMFDataStoragePrivate* d;
};

#endif // SYMBIANQMFDATASTORAGE_H

// End of File
