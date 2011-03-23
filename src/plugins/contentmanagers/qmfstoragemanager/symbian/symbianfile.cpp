#include "symbianfile.h"

#include <QString>

class SymbianFilePrivate : public QSharedData
{
public:
    SymbianFilePrivate();
    SymbianFilePrivate(const SymbianFilePrivate &other);
    ~SymbianFilePrivate();

    SymbianFilePrivate& operator=(const SymbianFilePrivate& other);

    mutable SymbianQMFDataStorage m_dataStorage;
    RFile m_file;
    QString m_name;
};

SymbianFilePrivate::SymbianFilePrivate()
{
    m_dataStorage.connect();
}

SymbianFilePrivate::SymbianFilePrivate(const SymbianFilePrivate &other)
    : QSharedData(other),
      m_dataStorage(other.m_dataStorage),
      m_file(other.m_file),
      m_name(other.m_name)
{
}

SymbianFilePrivate::~SymbianFilePrivate()
{
    m_file.Close();
}

SymbianFilePrivate& SymbianFilePrivate::operator=(const SymbianFilePrivate& other)
{
    if (this != &other) {
        m_file.Close();
        m_dataStorage = other.m_dataStorage;
        m_file = other.m_file;
        m_name = other.m_name;
    }

    return *this;
}

SymbianFile::SymbianFile()
{
    d = new SymbianFilePrivate;
}

SymbianFile::SymbianFile(const QString &name)
{
    d = new SymbianFilePrivate;
    d->m_name = name;
}

SymbianFile::SymbianFile(const SymbianFile& other)
    : d(other.d)
{
}

SymbianFile::~SymbianFile()
{
}

SymbianFile& SymbianFile::operator=(const SymbianFile& other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool SymbianFile::rename(const QString &newName)
{
    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(newName.utf16()));
    if (d->m_file.Rename(stringPtr) != KErrNone) {
        return false;
    }
    return true;
}

bool SymbianFile::open(QIODevice::OpenMode flags)
{
    if ((flags | QIODevice::WriteOnly) || (flags | QIODevice::ReadWrite)) {
        return d->m_dataStorage.openOrCreateProtectedFile(d->m_file, d->m_name);
    } else {
        return d->m_dataStorage.openProtectedFile(d->m_file, d->m_name);
    }
}

void SymbianFile::close()
{
    d->m_file.Close();
}

bool SymbianFile::flush()
{
    if (d->m_file.Flush() != KErrNone) {
        return false;
    }
    return true;
}

bool SymbianFile::exists(const QString &fileName)
{
    if (fileName.isEmpty()) {
        return false;
    }
    SymbianQMFDataStorage storage;
    storage.connect();
    return storage.protectedFileExists(fileName);
}

bool SymbianFile::remove(const QString &fileName)
{
    SymbianQMFDataStorage storage;
    storage.connect();
    return storage.removeProtectedFile(fileName);
}

bool SymbianFile::rename(const QString &oldName, const QString &newName)
{
    SymbianQMFDataStorage storage;
    storage.connect();
    return storage.renameProtectedFile(oldName, newName);
}

QByteArray SymbianFile::readAll()
{
    QByteArray byteArray;

    TInt fileSize;
    d->m_file.Size(fileSize);
    byteArray.resize(fileSize);
    TPtr8 data(reinterpret_cast<TUint8*>(byteArray.data()), fileSize);
    TInt retVal = d->m_file.Read(0, data);

    return byteArray;
}

qint64 SymbianFile::write(const QByteArray &byteArray)
{
    TInt retVal = d->m_file.Write(TPtrC8(reinterpret_cast<const TUint8*>(byteArray.data()), byteArray.size()));
    if (retVal != KErrNone) {
        return -1;
    }
    return byteArray.size();
}

// End of File
