#ifndef SYMBIANFILE_H
#define SYMBIANFILE_H

#include "qmfdatastorage.h"
#include <f32file.h>
#include <QSharedDataPointer>
#include <QIODevice>

class QString;
class SymbianFilePrivate;

class SymbianFile : public QObject
{
    Q_OBJECT

public:
    SymbianFile();
    SymbianFile(const QString &name);
    SymbianFile(const SymbianFile& other);
    ~SymbianFile();

    SymbianFile& operator=(const SymbianFile& other);

    bool open(QIODevice::OpenMode flags);
    virtual void close();

    bool rename(const QString &newName);
    bool flush();

    static bool exists(const QString &fileName);
    static bool remove(const QString &fileName);
    static bool rename(const QString &oldName, const QString &newName);

    QByteArray readAll();
    qint64 write(const QByteArray &byteArray);

private: // Data
    QSharedDataPointer<SymbianFilePrivate> d;
};

#endif // SYMBIANFILE_H

// End of File
