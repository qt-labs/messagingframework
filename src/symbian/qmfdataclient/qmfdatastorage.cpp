#include "qmfdatastorage.h"
#include "qmfdatasession.h"
#include "qmfdataclientservercommon.h"
#include <QString>
#include <QStringList>
#include <QThreadStorage>

class SymbianQMFDataStoragePrivate
{
public:
    SymbianQMFDataStoragePrivate();
    ~SymbianQMFDataStoragePrivate();

    static SymbianQMFDataStoragePrivate* instance();
    static void releaseInstance();

public: // Data
    int m_instanceCounter;
    bool m_connected;
    RQMFDataSession m_session;
};

Q_GLOBAL_STATIC(QThreadStorage<SymbianQMFDataStoragePrivate *>, qmfDataStoragePrivate)

SymbianQMFDataStoragePrivate::SymbianQMFDataStoragePrivate()
    : m_instanceCounter(0),
      m_connected(false)
{
}

SymbianQMFDataStoragePrivate::~SymbianQMFDataStoragePrivate()
{
    m_session.Close();
}

SymbianQMFDataStoragePrivate* SymbianQMFDataStoragePrivate::instance()
{
    if (!qmfDataStoragePrivate()->hasLocalData()) {
        qmfDataStoragePrivate()->setLocalData(new SymbianQMFDataStoragePrivate);
    }

    SymbianQMFDataStoragePrivate* instance = qmfDataStoragePrivate()->localData();
    instance->m_instanceCounter++;

    return instance;
}

void SymbianQMFDataStoragePrivate::releaseInstance()
{
    if (qmfDataStoragePrivate()->hasLocalData()) {
        SymbianQMFDataStoragePrivate* instance = qmfDataStoragePrivate()->localData();
        instance->m_instanceCounter--;
        if (instance->m_instanceCounter == 0) {
            qmfDataStoragePrivate()->setLocalData(0);
        }
    }
}

SymbianQMFDataStorage::SymbianQMFDataStorage()
    : d(SymbianQMFDataStoragePrivate::instance())
{
}

SymbianQMFDataStorage::SymbianQMFDataStorage(const SymbianQMFDataStorage& /*other*/)
    : d(SymbianQMFDataStoragePrivate::instance())
{
}

SymbianQMFDataStorage::~SymbianQMFDataStorage()
{
    d = 0;
    SymbianQMFDataStoragePrivate::releaseInstance();
}

SymbianQMFDataStorage& SymbianQMFDataStorage::operator=(const SymbianQMFDataStorage& /*other*/)
{
    return *this;
}

bool SymbianQMFDataStorage::connect()
{
    if (!d->m_connected) {
        if (d->m_session.Connect() == KErrNone) {
            d->m_connected = true;
        }
    }
    return d->m_connected;
}

bool SymbianQMFDataStorage::createDatabase(QString name)
{
    if (!d->m_connected) {
        return false;
    }

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(name.utf16()));
    return d->m_session.CreateDatabase(stringPtr);
}

bool SymbianQMFDataStorage::openProtectedFile(RFile& file, QString filePath)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtFilePathToSymbianPath(filePath);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(filePath.utf16()));
    return d->m_session.OpenFile(file, stringPtr);
}

bool SymbianQMFDataStorage::openOrCreateProtectedFile(RFile& file, QString filePath)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtFilePathToSymbianPath(filePath);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(filePath.utf16()));
    return d->m_session.OpenOrCreateFile(file, stringPtr);
}

bool SymbianQMFDataStorage::protectedFileExists(QString filePath)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtFilePathToSymbianPath(filePath);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(filePath.utf16()));
    return d->m_session.FileExists(stringPtr);
}

bool SymbianQMFDataStorage::removeProtectedFile(QString filePath)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtFilePathToSymbianPath(filePath);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(filePath.utf16()));
    return d->m_session.RemoveFile(stringPtr);
}

bool SymbianQMFDataStorage::renameProtectedFile(QString oldFilePath, QString newFilePath)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtFilePathToSymbianPath(oldFilePath);
    convertQtFilePathToSymbianPath(newFilePath);

    TPtrC16 stringPtr1(KNullDesC);
    stringPtr1.Set(reinterpret_cast<const TUint16*>(oldFilePath.utf16()));
    TPtrC16 stringPtr2(KNullDesC);
    stringPtr2.Set(reinterpret_cast<const TUint16*>(newFilePath.utf16()));
    return d->m_session.RenameFile(stringPtr1, stringPtr2);
}

bool SymbianQMFDataStorage::protectedDirectoryExists(QString path)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtPathToSymbianPath(path);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(path.utf16()));
    return d->m_session.DirectoryExists(stringPtr);
}

bool SymbianQMFDataStorage::makeProtectedDirectory(QString path)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtPathToSymbianPath(path);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(path.utf16()));
    return d->m_session.MakeDirectory(stringPtr);
}

bool SymbianQMFDataStorage::removeProtectedDirectory(QString path)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtPathToSymbianPath(path);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(path.utf16()));
    return d->m_session.RemoveDirectory(stringPtr);
}

bool SymbianQMFDataStorage::removeProtectedPath(QString path)
{
    if (!d->m_connected) {
        return false;
    }

    convertQtPathToSymbianPath(path);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(path.utf16()));
    return d->m_session.RemovePath(stringPtr);
}

QStringList SymbianQMFDataStorage::listDirectoryEntries(QString path)
{
    QStringList retVal;

    if (!d->m_connected) {
        return retVal;
    }

    convertQtPathToSymbianPath(path);

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(path.utf16()));
    RBuf directoryListing;
    if (d->m_session.DirectoryListing(stringPtr, directoryListing)) {
        QString directories = QString::fromUtf16(directoryListing.Ptr(), directoryListing.Length());
        directoryListing.Close();
        retVal = directories.split("/", QString::SkipEmptyParts);
    }

    return retVal;
}

void SymbianQMFDataStorage::convertQtFilePathToSymbianPath(QString& filePath)
{
    if (filePath.startsWith('/')) {
        filePath.remove(0,1);
    }
    filePath.replace('/','\\');
}

void SymbianQMFDataStorage::convertQtPathToSymbianPath(QString& path)
{
    if (path.startsWith('/')) {
        path.remove(0,1);
    }
    if (!path.endsWith('/')) {
        path.append('\\');
    }
    path.replace('/','\\');
}

// End of File
