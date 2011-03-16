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
