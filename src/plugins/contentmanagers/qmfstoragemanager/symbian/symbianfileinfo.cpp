#include "symbianfileinfo.h"

#include "symbiandir.h"
#include <bautils.h>
#include <QString>

SymbianFileInfo::SymbianFileInfo(const QString &file)
    : m_file(file)
{
    m_dataStorage.connect();
}

SymbianFileInfo::~SymbianFileInfo()
{
}

QString SymbianFileInfo::absoluteFilePath() const
{
    return m_file;
}

QString SymbianFileInfo::fileName() const
{
    return m_file.mid(m_file.lastIndexOf('/')+1);
}

SymbianDir SymbianFileInfo::dir() const
{
    int index = m_file.lastIndexOf('/');
    if (index == -1) {
        return SymbianDir(".");
    }
    return m_file.mid(0,index);
}

bool SymbianFileInfo::isRelative() const
{
    if (m_file.startsWith(QLatin1Char('/'))) {
        return false;
    }

    if (m_file.length() >= 2) {
        if (m_file.at(0).isLetter() && m_file.at(1) == QLatin1Char(':')) {
            return false;
        }
    }

    return true;
}

bool SymbianFileInfo::isDir() const
{
    return m_dataStorage.protectedDirectoryExists(m_file);
}

// End of File
