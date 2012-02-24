/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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
