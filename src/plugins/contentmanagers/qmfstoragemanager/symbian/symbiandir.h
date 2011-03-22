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
