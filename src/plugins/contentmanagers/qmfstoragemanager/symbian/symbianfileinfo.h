#ifndef SYMBIANFILEINFO_H
#define SYMBIANFILEINFO_H

#include "qmfdatastorage.h"
#include <QString>

class SymbianDir;

class SymbianFileInfo
{
public:
    SymbianFileInfo(const QString &file);
    ~SymbianFileInfo();

    QString absoluteFilePath() const;
    QString fileName() const;
    SymbianDir dir() const;

    bool isRelative() const;
    inline bool isAbsolute() const { return !isRelative(); }
    bool isDir() const;

private: // Data
    mutable SymbianQMFDataStorage m_dataStorage;
    QString m_file;
};

#endif // SYMBIANFILEINFO_H

// End of File
