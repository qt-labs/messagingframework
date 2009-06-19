/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtopiamailfilemanager.h"
#include "qmailmessage.h"
#include "qmailstore.h"
#include "qmailnamespace.h"
#include "qmaillog.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QtPlugin>
#include <QUrl>
#if defined(Q_OS_WIN)
#include <windows.h>
#include <io.h>
#elif defined(Q_OS_UNIX)
#include <unistd.h>
#endif

namespace {

const QString gKey("qtopiamailfile");

QMap<QMailAccountId, QString> gAccountPath;

QString defaultPath()
{
    QString path = QMail::dataPath();
    if (!path.endsWith("/"))
        path.append("/");
    path.append("mail");

    return path;
}

QString randomString(int length)
{
    if (length <= 0) 
        return QString();

    QString str;
    str.resize(length);

    int i = 0;
    while (length--) {
        int r=qrand() % 62;
        r+=48;
        if (r>57) r+=7;
        if (r>90) r+=6;
        str[i++] =  char(r);
    }

    return str;
}

QString generateUniqueFileName(const QMailAccountId &accountId, const QString &name = QString())
{
    static const quint64 pid = QCoreApplication::applicationPid();

    QString filename;

    // Format: [name]seconds_epoch.pid.randomchars
    filename = name;
    filename.append(QString::number(QDateTime::currentDateTime().toTime_t()));
    filename.append('.');
    filename.append(QString::number(pid));
    filename.append('.');

    while (true) {
        QString path = QtopiamailfileManager::messageFilePath(filename + randomString(5), accountId);
        if (!QFile::exists(path)) {
            return path;
        }
    }

    return QString();
}

void recursivelyRemovePath(const QString &path, bool preserveTopDirectory = true)
{
    QFileInfo fi(path);
    if (!fi.isDir()) {
        QFile::remove(path);
        return;
    }

    QDir dir(path);
    foreach (const QString &file, dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        recursivelyRemovePath(path + "/" + file, false);
    }

    if (!preserveTopDirectory) {
        dir.setPath("/");
        dir.rmpath(path);
    }
}

bool pathOnDefault(const QString &path)
{
    return path.startsWith(defaultPath());
}

bool migrateAccountToVersion101(const QMailAccountId &accountId)
{
    foreach (const QMailMessageId &id, QMailStore::instance()->queryMessages(QMailMessageKey::parentAccountId(accountId))) {
        const QMailMessage message(id);

        if (message.multipartType() != QMailMessage::MultipartNone) {
            // Any part files for this message need to be moved to the new location
            QString fileName(message.contentIdentifier());

            QFileInfo fi(fileName);
            QDir path(fi.dir());
            path.setNameFilters(QStringList() << (fi.fileName() + "-[0-9]*"));

            QStringList entries(path.entryList());
            if (!entries.isEmpty()) {
                // Does the part directory exist?
                QString partDirectory(QtopiamailfileManager::messagePartDirectory(fileName));
                if (!QDir(partDirectory).exists()) {
                    if (!QDir::root().mkpath(partDirectory)) {
                        qMailLog(Messaging) << "Unable to create directory for message part content:" << partDirectory;
                        return false;
                    }
                }

                foreach (const QString &entry, entries) {
                    QFile existing(path.filePath(entry));

                    // We need to move this file to the subdirectory
                    QString newName(partDirectory + '/');
                    int index = entry.lastIndexOf('-');
                    if (index != -1) {
                        newName.append(entry.mid(index + 1));
                    }

                    if (!existing.rename(newName)) {
                        qMailLog(Messaging) << "Unable to move part file to new name:" << newName;
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void sync(QFile *file)
{
    // Ensure data is flushed to OS before attempting sync
    file->flush();

#if defined(Q_OS_WIN)
    ::FlushFileBuffers(reinterpret_cast<HANDLE>(::_get_osfhandle(file->handle())));
#elif defined(Q_OS_UNIX)
#if defined(_POSIX_SYNCHRONIZED_IO) && (_POSIX_SYNCHRONIZED_IO > 0)
    ::fdatasync(file->handle());
#else
    ::fsync(file->handle());
#endif
#endif
}

}


QtopiamailfileManager::QtopiamailfileManager()
{
    QString path(messagesBodyPath(QMailAccountId()));

    // Make sure messages body path exists
    QDir dir(path);
    if (!dir.exists() && !dir.mkpath(path)) {
        qMailLog(Messaging) << "Unable to create messages storage directory " << path;
    }

    if (QMailStore *store = QMailStore::instance()) {
        connect(store, SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(clearAccountPath(QMailAccountIdList)));
        connect(store, SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(clearAccountPath(QMailAccountIdList)));
    }
}

QtopiamailfileManager::~QtopiamailfileManager()
{
    // Finalise any changes we have pending
    ensureDurability();
}

void QtopiamailfileManager::clearAccountPath(const QMailAccountIdList &ids)
{
    foreach (const QMailAccountId &id, ids)
        gAccountPath.remove(id);
}

QMailStore::ErrorCode QtopiamailfileManager::add(QMailMessage *message, QMailContentManager::DurabilityRequirement durability)
{
    return addOrRename(message, QString(), (durability == QMailContentManager::EnsureDurability));
}

QMailStore::ErrorCode QtopiamailfileManager::addOrRename(QMailMessage *message, const QString &existingIdentifier, bool durable)
{
    QString filePath;
    if (message->contentIdentifier().isEmpty()) {
        filePath = generateUniqueFileName(message->parentAccountId());
    } else {
        // Use the supplied identifier as a filename
        filePath = generateUniqueFileName(message->parentAccountId(), message->contentIdentifier());
    }

    message->setContentIdentifier(filePath);

    QFile *file = new QFile(filePath);

    QString detachedFile = message->customField("qtopiamail-detached-filename");
    if (!detachedFile.isEmpty() && (message->multipartType() == QMailMessagePartContainer::MultipartNone)) {
        // Try to take ownership of the file
        if (QFile::rename(detachedFile, filePath)) {
            message->removeCustomField("qtopiamail-detached-filename");
            return QMailStore::NoError;
        }
    }

    if (!file->open(QIODevice::WriteOnly)) {
        qMailLog(Messaging) << "Unable to open new message content file:" << filePath;
        return (pathOnDefault(filePath) ? QMailStore::FrameworkFault : QMailStore::ContentInaccessible);
    }

    // Write the message to file (not including sub-part contents)
    QDataStream out(file);
    message->toRfc2822(out, QMailMessage::StorageFormat);
    if ((out.status() != QDataStream::Ok) ||
        // Write each part to file
        ((message->multipartType() != QMailMessagePartContainer::MultipartNone) &&
         !addOrRenameParts(message, message->contentIdentifier(), existingIdentifier, durable))) {
        // Remove the file
        file->close();
        qMailLog(Messaging) << "Unable to save message content, removing temporary file:" << filePath;
        if (!QFile::remove(filePath)){
            qMailLog(Messaging) << "Unable to remove temporary message content file:" << filePath;
        }

        // Try to remove any parts that were created
        removeParts(message->contentIdentifier());

        return QMailStore::FrameworkFault;
    }

    if (durable) {
        sync(file);
        delete file;
    } else {
        _openFiles.append(file);
    }

    message->removeCustomField("qtopiamail-detached-filename");
    return QMailStore::NoError;
}

QMailStore::ErrorCode QtopiamailfileManager::update(QMailMessage *message, QMailContentManager::DurabilityRequirement durability)
{
    QString existingIdentifier(message->contentIdentifier());

    // Store to a new file
    message->setContentIdentifier(QString());
    QMailStore::ErrorCode code = addOrRename(message, existingIdentifier, (durability == QMailContentManager::EnsureDurability));
    if (code != QMailStore::NoError) {
        message->setContentIdentifier(existingIdentifier);
        return code;
    } 

    if (!existingIdentifier.isEmpty()) {
        // Try to remove the existing data
        code = remove(existingIdentifier);
        if (code != QMailStore::NoError) {
            qMailLog(Messaging) << "Unable to remove superseded message content:" << existingIdentifier;
            return code;
        }
    }

    return QMailStore::NoError;
}

QMailStore::ErrorCode QtopiamailfileManager::ensureDurability()
{
    foreach (QFile *file, _openFiles) {
        sync(file);
        delete file;
    }

    _openFiles.clear();
    return QMailStore::NoError;
}

QMailStore::ErrorCode QtopiamailfileManager::remove(const QString &identifier)
{
    QMailStore::ErrorCode result(QMailStore::NoError);

    QFileInfo fi(identifier);
    QString path(fi.absoluteFilePath());
    if (QFile::exists(path) && !QFile::remove(path)) {
        qMailLog(Messaging) << "Unable to remove content file:" << identifier;
        result = QMailStore::ContentNotRemoved;
    }

    if (!removeParts(identifier)) {
        qMailLog(Messaging) << "Unable to remove part content files for:" << identifier;
        result = QMailStore::ContentNotRemoved;
    }

    return result;
}

struct ReferenceLoader
{
    const QMailMessage *message;

    ReferenceLoader(const QMailMessage *m) : message(m) {}

    bool operator()(QMailMessagePart &part)
    {
        QString loc = part.location().toString(false);

        // See if this part is a reference
        QString name("qtopiamail-reference-location-" + loc);
        QString value(message->customField(name));
        if (!value.isEmpty()) {
            // This part is a reference
            QString reference;
            int index = value.indexOf(':');
            if (index != -1) {
                reference = value.mid(index + 1);
                QString referenceType = value.left(index);

                if (referenceType == "part") {
                    part.setReference(QMailMessagePart::Location(reference), part.contentType(), part.transferEncoding());
                } else if (referenceType == "message") {
                    part.setReference(QMailMessageId(reference.toULongLong()), part.contentType(), part.transferEncoding());
                }
            }

            if (reference.isEmpty() || (part.referenceType() == QMailMessagePart::None)) {
                qMailLog(Messaging) << "Unable to resolve reference from:" << value;
                return false;
            }

            // Is this reference resolved?
            name = QString("qtopiamail-reference-resolution-" + loc);
            value = message->customField(name);
            if (!value.isEmpty()) {
                part.setReferenceResolution(value);
            }
        }

        return true;
    }
};

struct PartLoader
{
    QString fileName;

    PartLoader(const QString &path) : fileName(path) {}

    bool operator()(QMailMessagePart &part)
    {
        if ((part.referenceType() == QMailMessagePart::None) &&
            (part.multipartType() == QMailMessagePartContainer::MultipartNone)) {
            QString partFilePath;

            bool localAttachment = QFile::exists(QUrl(part.contentLocation()).toLocalFile()) && !part.hasBody();
            if (localAttachment)
                partFilePath = QUrl(part.contentLocation()).toLocalFile();
            else
                partFilePath = QtopiamailfileManager::messagePartFilePath(part, fileName);

            if (QFile::exists(partFilePath)) {
                // Is the file content in encoded or decoded form?  Since we're delivering
                // server-side data, the parameter seems reversed...
                QMailMessageBody::EncodingStatus dataState(part.contentAvailable() ? QMailMessageBody::AlreadyEncoded : QMailMessageBody::RequiresEncoding);
                part.setBody(QMailMessageBody::fromFile(partFilePath, part.contentType(), part.transferEncoding(), dataState));
                if (!part.hasBody())
                    return false;
            }
        }

        return true;
    }
};

QMailStore::ErrorCode QtopiamailfileManager::load(const QString &identifier, QMailMessage *message)
{
    QString path(identifier);
    if (!QFile::exists(path)) {
        if (!QFileInfo(path).isAbsolute()) {
            // See if this is a relative path from the time before paths were modifiable
            QString adjustedPath(messageFilePath(identifier, QMailAccountId()));
            if (QFile::exists(adjustedPath)) {
                path = adjustedPath;
            }
        }
    }
    if (!QFile::exists(path)) {
        qMailLog(Messaging) << "Unable to load nonexistent content file:" << identifier;
        return (pathOnDefault(path) ? QMailStore::FrameworkFault : QMailStore::ContentInaccessible);
    }

    QMailMessage result(QMailMessage::fromRfc2822File(path));

    // Load the reference information from the meta data into our content object
    ReferenceLoader refLoader(message);
    if (!result.foreachPart<ReferenceLoader&>(refLoader)) {
        qMailLog(Messaging) << "Unable to resolve references for:" << identifier;
        return QMailStore::FrameworkFault;
    }

    // Load the content of each part
    PartLoader partLoader(path);
    if (!result.foreachPart<PartLoader&>(partLoader)) {
        qMailLog(Messaging) << "Unable to load parts for:" << identifier;
        return QMailStore::FrameworkFault;
    }

    *message = result;
    return QMailStore::NoError;
}

bool QtopiamailfileManager::init()
{
    // It used to be possible for accounts to not have a storage service configured.
    // If so, add a configuration to those accounts for qtopiamailfile
    foreach (const QMailAccountId &accountId, QMailStore::instance()->queryAccounts()) {
        QMailAccountConfiguration config(accountId);

        if (!config.services().contains(gKey)) {
            bool storageConfigured(false);

            // See if any of these services are for storage
            foreach (const QString &service, config.services()) {
                QMailAccountConfiguration::ServiceConfiguration &svcCfg(config.serviceConfiguration(service));

                if (svcCfg.value("servicetype") == "storage") {
                    storageConfigured = true;
                    break;
                }
            }

            if (!storageConfigured) {
                // Add a configuration for our default service
                config.addServiceConfiguration(gKey);

                QMailAccountConfiguration::ServiceConfiguration &svcCfg(config.serviceConfiguration(gKey));
                svcCfg.setValue("version", "100");
                svcCfg.setValue("servicetype", "storage");

                if (QMailStore::instance()->updateAccountConfiguration(&config)) {
                    qMailLog(Messaging) << "Added storage configuration for account" << accountId;
                } else {
                    qWarning() << "Unable to add missing storage configuration for account:" << accountId;
                    return false;
                }
            }
        }
    }

    // Migrate any data in older formats
    foreach (const QMailAccountId &accountId, QMailStore::instance()->queryAccounts()) {
        QMailAccountConfiguration config(accountId);

        if (config.services().contains(gKey)) {
            // This account uses our content manager
            QMailAccountConfiguration::ServiceConfiguration &svcCfg = config.serviceConfiguration(gKey);
            int version = svcCfg.value("version").toInt();

            if (version == 100) {
                // Version 100 - part files are not in subdirectories
                if (!migrateAccountToVersion101(accountId)) {
                    qWarning() << "Unable to migrate account data to version 101 for account:" << accountId;
                    return false;
                }

                version = 101;
            }

            if (svcCfg.value("version").toInt() != version) {
                svcCfg.setValue("version", QString::number(version));

                if (QMailStore::instance()->updateAccountConfiguration(&config)) {
                    qMailLog(Messaging) << "Migrated content data for account" << accountId << "to version" << version;
                } else {
                    qWarning() << "Unable to update account configuration for account:" << accountId;
                    return false;
                }
            }
        }
    }

    return true;
}

void QtopiamailfileManager::clearContent()
{
    // Delete all content files
    recursivelyRemovePath(messagesBodyPath(QMailAccountId()));
}

const QString &QtopiamailfileManager::messagesBodyPath(const QMailAccountId &accountId)
{
    static QString path(defaultPath());

    if (accountId.isValid()) {
        QMap<QMailAccountId, QString>::const_iterator it = gAccountPath.find(accountId);
        if (it == gAccountPath.end()) {
            QString basePath;

            // Look up the configuration for this account
            QMailAccountConfiguration config(accountId);

            if (config.services().contains(gKey)) {
                QMailAccountConfiguration::ServiceConfiguration &svcCfg = config.serviceConfiguration(gKey);
                basePath = svcCfg.value("basePath");
            }

            it = gAccountPath.insert(accountId, basePath);
        }

        if (!it.value().isEmpty())
            return it.value();
    }

    return path;
}

QString QtopiamailfileManager::messageFilePath(const QString &fileName, const QMailAccountId &accountId)
{
    return messagesBodyPath(accountId) + "/" + fileName;
}

QString QtopiamailfileManager::messagePartFilePath(const QMailMessagePart &part, const QString &fileName)
{
    return messagePartDirectory(fileName) + '/' + part.location().toString(false); 
}

QString QtopiamailfileManager::messagePartDirectory(const QString &fileName)
{
    return fileName + "-parts";
}

struct PartStorer
{
    QMailMessage *message;
    QString fileName;
    QString existing;
    QList<QFile*> *openFiles;

    PartStorer(QMailMessage *m, const QString &f, const QString &e, QList<QFile*> *o) : message(m), fileName(f), existing(e), openFiles(o) {}

    bool operator()(const QMailMessagePart &part)
    {
        if ((part.referenceType() == QMailMessagePart::None) &&
            (part.multipartType() == QMailMessagePartContainer::MultipartNone) &&
            part.hasBody()) {
            // We need to store this part
            QString partFilePath(QtopiamailfileManager::messagePartFilePath(part, fileName));

            if (!part.contentModified() && !existing.isEmpty()) {
                // This part is not modified; see if we can simply move the existing file to the new identifier
                QString existingPath(QtopiamailfileManager::messagePartFilePath(part, existing));
                if (QFile::rename(existingPath, partFilePath)) {
                    return true;
                }
            }

            // We can only write the content in decoded form if it is complete
            QMailMessageBody::EncodingFormat outputFormat(part.contentAvailable() ? QMailMessageBody::Decoded : QMailMessageBody::Encoded);

            QString detachedFile = message->customField("qtopiamail-detached-filename");
            if (!detachedFile.isEmpty()) {
                // We can take ownership of the file if that helps
                if ((outputFormat == QMailMessageBody::Encoded) ||
                    // If the output should be decoded, then only unchanged 'encodings 'can be used directly
                    ((part.transferEncoding() != QMailMessageBody::Base64) &&
                     (part.transferEncoding() != QMailMessageBody::QuotedPrintable))) {
                    // Try to take ownership of the file
                    if (QFile::rename(detachedFile, partFilePath)) {
                        message->removeCustomField("qtopiamail-detached-filename");
                        return true;
                    }
                }
            }

            // We need to write the content to a new file
            QFile *file = new QFile(partFilePath);
            if (!file->open(QIODevice::WriteOnly)) {
                qMailLog(Messaging) << "Unable to open new message part content file:" << partFilePath;
                return false;
            }

            // Write the part content to file
            QDataStream out(file);
            if (!part.body().toStream(out, outputFormat) || (out.status() != QDataStream::Ok)) {
                qMailLog(Messaging) << "Unable to save message part content, removing temporary file:" << partFilePath;
                file->close();
                if (!QFile::remove(partFilePath)){
                    qMailLog(Messaging) << "Unable to remove temporary message part content file:" << partFilePath;
                }
                return false;
            }

            if (openFiles) {
                openFiles->append(file);
            } else {
                sync(file);
                delete file;
            }
        }

        return true;
    }
};

bool QtopiamailfileManager::addOrRenameParts(QMailMessage *message, const QString &fileName, const QString &existing, bool durable)
{
    // Ensure that the part directory exists
    QString partDirectory(messagePartDirectory(fileName));
    if (!QDir(partDirectory).exists()) {
        if (!QDir::root().mkpath(partDirectory)) {
            qMailLog(Messaging) << "Unable to create directory for message part content:" << partDirectory;
            return false;
        }
    }

    PartStorer partStorer(message, fileName, existing, (durable ? 0 : &_openFiles));
    if (!const_cast<const QMailMessage*>(message)->foreachPart(partStorer)) {
        qMailLog(Messaging) << "Unable to store parts for message:" << fileName;
        return false;
    }

    return true;
}

bool QtopiamailfileManager::removeParts(const QString &fileName)
{
    bool result(true);

    QString partDirectory(messagePartDirectory(fileName));

    QDir dir(partDirectory);
    if (dir.exists()) {
        // Remove any files in this directory
        foreach (const QString &entry, dir.entryList()) {
            if ((entry != QLatin1String(".")) && (entry != QLatin1String(".."))) {
                if (!dir.remove(entry)) {
                    qMailLog(Messaging) << "Unable to remove part file:" << entry;
                    result = false;
                }
            }
        }

        if (!QDir::root().rmdir(dir.absolutePath())) {
            qMailLog(Messaging) << "Unable to remove directory for message part content:" << partDirectory;
            result = false;
        }
    }

    return result;
}

Q_EXPORT_PLUGIN2(qtopiamailfilemanager,QtopiamailfileManagerPlugin)

QtopiamailfileManagerPlugin::QtopiamailfileManagerPlugin()
    : QMailContentManagerPlugin()
{
}

QString QtopiamailfileManagerPlugin::key() const
{
    return gKey;
}

QMailContentManager *QtopiamailfileManagerPlugin::create()
{
    return new QtopiamailfileManager;
}

