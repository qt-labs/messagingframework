/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qtopiamailfilemanager.h"
#include "qmailmessage.h"
#include "qmailstore.h"
#include <qmailnamespace.h>
#include <qmaillog.h>
#include <QFile>
#include <QDir>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <time.h>
#include <QtPlugin>
#include <QUrl>

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
    // Format: seconds_epoch.pid.randomchars
    bool exists = true;
    const qint64 pid = ::getpid();

    QString filename;
    QString path;

    while (exists) {
        filename = name;
        filename.sprintf("%ld.%ld.",(unsigned long)time(0), (long)pid);
        filename.prepend(name);
        filename.append(randomString(5));
        path = QtopiamailfileManager::messageFilePath(filename, accountId);

        exists = QFile::exists(path);
    }

    return path;
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

void sync(const QFile &file)
{
#if defined(_POSIX_SYNCHRONIZED_IO) && (_POSIX_SYNCHRONIZED_IO > 0)
    ::fdatasync(file.handle());
#else
    ::fsync(file.handle());
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

void QtopiamailfileManager::clearAccountPath(const QMailAccountIdList &ids)
{
    foreach (const QMailAccountId &id, ids)
        gAccountPath.remove(id);
}

QMailStore::ErrorCode QtopiamailfileManager::add(QMailMessage *message)
{
    return addOrRename(message, QString());
}

QMailStore::ErrorCode QtopiamailfileManager::addOrRename(QMailMessage *message, const QString &existingIdentifier)
{
    QString filePath;
    if (message->contentIdentifier().isEmpty()) {
        filePath = generateUniqueFileName(message->parentAccountId());
    } else {
        // Use the supplied identifier as a filename
        filePath = generateUniqueFileName(message->parentAccountId(), message->contentIdentifier());
    }

    message->setContentIdentifier(filePath);

    QFile file(filePath);

    QString detachedFile = message->customField("qtopiamail-detached-filename");
    if (!detachedFile.isEmpty() && (message->multipartType() == QMailMessagePartContainer::MultipartNone)) {
        // Try to take ownership of the file
        if (QFile::rename(detachedFile, filePath)) {
            message->removeCustomField("qtopiamail-detached-filename");
            return QMailStore::NoError;
        }
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qMailLog(Messaging) << "Unable to open new message content file:" << filePath;
        return (pathOnDefault(filePath) ? QMailStore::FrameworkFault : QMailStore::ContentInaccessible);
    }

    // Write the message to file (not including sub-part contents)
    QDataStream out(&file);
    message->toRfc2822(out, QMailMessage::StorageFormat);
    bool isOk = out.status() != QDataStream::Ok;
    if ((out.status() != QDataStream::Ok) ||
        // Write each part to file
        ((message->multipartType() != QMailMessagePartContainer::MultipartNone) &&
         !addOrRenameParts(message, *message, message->contentIdentifier(), existingIdentifier))) {
        // Remove the file
        qMailLog(Messaging) << "Unable to save message content, removing temporary file:" << filePath;
        if (!QFile::remove(filePath)){
            qMailLog(Messaging) << "Unable to remove temporary message content file:" << filePath;
        }

        // Try to remove any parts that were created
        removeParts(message->contentIdentifier());

        return QMailStore::FrameworkFault;
    }

    sync(file);

    message->removeCustomField("qtopiamail-detached-filename");
    return QMailStore::NoError;
}

QMailStore::ErrorCode QtopiamailfileManager::update(QMailMessage *message)
{
    QString existingIdentifier(message->contentIdentifier());

    // Store to a new file
    message->setContentIdentifier(QString());
    QMailStore::ErrorCode code = addOrRename(message, existingIdentifier);
    if (code != QMailStore::NoError) {
        message->setContentIdentifier(existingIdentifier);
        return code;
    } 

    // Try to remove the existing data
    code = remove(existingIdentifier);
    if (code != QMailStore::NoError) {
        qMailLog(Messaging) << "Unable to remove superseded message content:" << existingIdentifier;
    }

    return QMailStore::NoError;
}

QMailStore::ErrorCode QtopiamailfileManager::remove(const QString &identifier)
{
    QMailStore::ErrorCode result(QMailStore::NoError);

    QFileInfo fi(identifier);
    QDir path(fi.dir());
    if (!path.remove(fi.fileName())) {
        qMailLog(Messaging) << "Unable to remove content file:" << identifier;
        result = QMailStore::FrameworkFault;
    }

    if (!removeParts(identifier)) {
        qMailLog(Messaging) << "Unable to remove part content files for:" << identifier;
        result = QMailStore::FrameworkFault;
    }

    return result;
}

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
    if (!loadParts(message, &result, path))
        return QMailStore::FrameworkFault;

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

static bool isLocalAttachment(const QMailMessagePart& part)
{
    QString contentLocation = part.contentLocation().remove(QRegExp("\\s"));
    return QFile::exists(QUrl(contentLocation).toLocalFile()) && !part.hasBody();
}

bool QtopiamailfileManager::addOrRenameParts(QMailMessage *message, const QMailMessagePartContainer &container, const QString &fileName, const QString &existing)
{
    // Ensure that the part directory exists
    QString partDirectory(messagePartDirectory(fileName));
    if (!QDir(partDirectory).exists()) {
        if (!QDir::root().mkpath(partDirectory)) {
            qMailLog(Messaging) << "Unable to create directory for message part content:" << partDirectory;
            return false;
        }
    }

    for (uint i = 0; i < container.partCount(); ++i) {
        const QMailMessagePart &part(container.partAt(i));
        QString loc = part.location().toString(false);

        if (part.referenceType() == QMailMessagePart::None) {
            if (part.multipartType() == QMailMessagePartContainer::MultipartNone) {
                if (part.hasBody()) {
                    QString partFilePath(messagePartFilePath(part, fileName));

                    bool additionRequired(part.contentModified() || existing.isEmpty());
                    if (!additionRequired) {
                        QString existingPath(messagePartFilePath(part, existing));

                        // This part is not modified; see if we can simply move the existing file to the new identifier
                        if (!QFile::rename(existingPath, partFilePath))
                            additionRequired = true;
                    }

                    if (additionRequired && !isLocalAttachment(part)) {
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
                                    continue;
                                }
                            }
                        }

                        QFile file(partFilePath);
                        if (!file.open(QIODevice::WriteOnly)) {
                            qMailLog(Messaging) << "Unable to open new message part content file:" << partFilePath;
                            return false;
                        }

                        // Write the part content to file
                        QDataStream out(&file);
                        if (!part.body().toStream(out, outputFormat) || (out.status() != QDataStream::Ok)) {
                            qMailLog(Messaging) << "Unable to save message part content, removing temporary file:" << partFilePath;
                            if (!QFile::remove(partFilePath)){
                                qMailLog(Messaging) << "Unable to remove temporary message part content file:" << partFilePath;
                            }

                            return false;
                        }

                        sync(file);
                    }
                }
            } else {
                // Write any sub-parts of this part out
                if (!addOrRenameParts(message, part, fileName, existing))
                    return false;
            }
        } else {
            // Mark this as a reference in the container message
            QString value;
            if (part.referenceType() == QMailMessagePart::PartReference) {
                value = "part:" + part.partReference().toString(true);
            } else if (part.referenceType() == QMailMessagePart::MessageReference) {
                value = "message:" + QString::number(part.messageReference().toULongLong());
            }

            QString name(gKey + "-reference-location-" + loc);
            message->setCustomField(name, value);
        }
    }

    return true;
}

bool QtopiamailfileManager::loadParts(QMailMessage *message, QMailMessagePartContainer *container, const QString &fileName)
{
    for (uint i = 0; i < container->partCount(); ++i) {
        QMailMessagePart &part(container->partAt(i));
        QString loc = part.location().toString(false);

        // See if this part is a reference
        QString name(gKey + "-reference-location-" + loc);
        QString value(message->customField(name));
        if (!value.isEmpty()) {
            // This part is just a reference
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
            }
        } else {
            if (part.multipartType() == QMailMessagePartContainer::MultipartNone) {
                QString partFilePath;
                bool localAttachment = QFile::exists(QUrl(part.contentLocation()).toLocalFile()) && !part.hasBody();
                if(localAttachment)
                    partFilePath = QUrl(part.contentLocation()).toLocalFile();
                else
                    partFilePath = (messagePartFilePath(part, fileName));

                if (QFile::exists(partFilePath)) {
                    // Is the file content in encoded or decoded form?  Since we're delivering
                    // server-side data, the parameter seems reversed...
                    QMailMessageBody::EncodingStatus dataState(part.contentAvailable() ? QMailMessageBody::AlreadyEncoded : QMailMessageBody::RequiresEncoding);
                    part.setBody(QMailMessageBody::fromFile(partFilePath, part.contentType(), part.transferEncoding(), dataState));
                    if (!part.hasBody())
                        return false;
                }
            } else {
                // Write any sub-parts of this part out
                if (!loadParts(message, &part, fileName))
                    return false;
            }
        }
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

