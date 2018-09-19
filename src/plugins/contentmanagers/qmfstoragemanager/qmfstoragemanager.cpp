/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmfstoragemanager.h"
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
#define USE_FSYNC_PER_FILE
#elif defined(Q_OS_UNIX)
#include <unistd.h>
#endif

namespace {

const QString gKey("qmfstoragemanager");

QMap<QMailAccountId, QString> gAccountPath;

QString defaultPath()
{
    QString path = QMail::dataPath();
    if (!path.endsWith('/'))
        path.append('/');
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

    QString path;
    do {
        path = QmfStorageManager::messageFilePath(filename + randomString(5), accountId);
    } while (QFile::exists(path));

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
        recursivelyRemovePath(path + '/' + file, false);
    }

    if (!preserveTopDirectory) {
        dir.setPath(QString('/'));
        dir.rmpath(path);
    }
}

bool pathOnDefault(const QString &path)
{
    return path.startsWith(defaultPath());
}

void syncFile(QSharedPointer<QFile> file)
{
    // Ensure data is flushed to OS before attempting sync
    file->flush();
#if defined(QMF_NO_DURABILITY) || defined(QMF_NO_SYNCHRONOUS_DB)
    // Durability is disabled
    return;
#endif

#if defined(Q_OS_WIN)
    ::FlushFileBuffers(reinterpret_cast<HANDLE>(::_get_osfhandle(file->handle())));
#elif defined(Q_OS_UNIX)
#if defined(_POSIX_SYNCHRONIZED_IO) && (_POSIX_SYNCHRONIZED_IO > 0)
    int handle(file->handle());
    if (handle != -1)
        ::fdatasync(handle);
    else
        qWarning() << "Could not get file handle for fdatasync";
#else
    ::fsync(file->handle());
#endif
#endif
}

}


QmfStorageManager::QmfStorageManager(QObject *parent)
    : QObject(parent),
      _useFullSync(false)
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

QmfStorageManager::~QmfStorageManager()
{
    // Finalise any changes we have pending
    ensureDurability();
}

void QmfStorageManager::clearAccountPath(const QMailAccountIdList &ids)
{
    foreach (const QMailAccountId &id, ids)
        gAccountPath.remove(id);
}

QMailStore::ErrorCode QmfStorageManager::add(QMailMessage *message, QMailContentManager::DurabilityRequirement durability)
{
    return addOrRename(message, QString(), durability);
}

QMailStore::ErrorCode QmfStorageManager::addOrRename(QMailMessage *message, const QString &existingIdentifier, QMailContentManager::DurabilityRequirement durability)
{
    // Use the supplied identifier as a filename
    QString filePath = generateUniqueFileName(message->parentAccountId());

    message->setContentIdentifier(filePath);

    QString detachedFile = message->customField("qmf-detached-filename");
    if (!detachedFile.isEmpty() && (message->multipartType() == QMailMessagePartContainer::MultipartNone)) {
        // Try to take ownership of the file
        if (QFile::rename(detachedFile, filePath)) {
            message->removeCustomField("qmf-detached-filename");
            return QMailStore::NoError;
        }
    }

    QSharedPointer<QFile> file(new QFile(filePath));

    if (!file->open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open new message content file:" << filePath;
        return (pathOnDefault(filePath) ? QMailStore::FrameworkFault : QMailStore::ContentInaccessible);
    }

    // Write the message to file (not including sub-part contents)
    QDataStream out(file.data());
    message->toRfc2822(out, QMailMessage::StorageFormat);
    if ((out.status() != QDataStream::Ok) ||
        // Write each part to file
        ((message->multipartType() != QMailMessagePartContainer::MultipartNone) &&
         !addOrRenameParts(message, message->contentIdentifier(), existingIdentifier, durability))) {
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

    if (durability == QMailContentManager::EnsureDurability) {
        syncFile(file);
    } else if (durability == QMailContentManager::DeferDurability) {
        syncLater(file);
    } // else NoDurability

    message->removeCustomField("qmf-detached-filename");
    if (!detachedFile.isEmpty()) {
        QFile::remove(detachedFile);
    }

    return QMailStore::NoError;
}

QMailStore::ErrorCode QmfStorageManager::update(QMailMessage *message, QMailContentManager::DurabilityRequirement durability)
{
    QString existingIdentifier(message->contentIdentifier());

    // Store to a new file
    message->setContentIdentifier(QString());
    QMailStore::ErrorCode code = addOrRename(message, existingIdentifier, durability);
    if (code != QMailStore::NoError) {
        message->setContentIdentifier(existingIdentifier);
        return code;
    }

    if (!existingIdentifier.isEmpty() && (durability != NoDurability)) {
        // Try to remove the existing data,
        // but if NoDurability don't delete existing content, it should be deleted later
        code = remove(existingIdentifier);
        if (code != QMailStore::NoError) {
            qMailLog(Messaging) << "Unable to remove superseded message content:" << existingIdentifier;
            return code;
        }
    }

    return QMailStore::NoError;
}

QMailStore::ErrorCode QmfStorageManager::ensureDurability()
{
    if (_useFullSync) {
        // More than one file needs to be synchronized
#if !defined(QMF_NO_DURABILITY) && !defined(QMF_NO_SYNCHRONOUS_DB)

        // Durability is not disabled
#if defined(Q_OS_WIN)
        qWarning() << "Unable to call sync on Windows.";
#else
        ::sync();
#endif
#endif
        _useFullSync = false;
    } else {
        foreach (QSharedPointer<QFile> file, _openFiles) {
            syncFile(file);
        }
    }
    _openFiles.clear();

    return QMailStore::NoError;
}

QMailStore::ErrorCode QmfStorageManager::ensureDurability(const QList<QString> &identifiers)
{
    Q_UNUSED(identifiers)
#if !defined(QMF_NO_DURABILITY) && !defined(QMF_NO_SYNCHRONOUS_DB)
    // Durability is not disabled
    
    // Can't just sync identifiers, also must sync message parts
#if defined(Q_OS_WIN)
            qWarning() << "Unable to call sync in ensureDurability.";
#else
            ::sync();
#endif
#endif
    return QMailStore::NoError;
}

QMailStore::ErrorCode QmfStorageManager::remove(const QString &identifier)
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
        QString name("qmf-reference-location-" + loc);
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
            name = QString("qmf-reference-resolution-" + loc);
            value = message->customField(name);
            if (!value.isEmpty()) {
                part.setReferenceResolution(value);
            }
        }

        return true;
    }
};

static bool loadPartUndecoded(QMailMessagePart &part, const QString &fileName)
{
    QString partFilePath = QmfStorageManager::messagePartUndecodedFilePath(part, fileName);
    QFile file(partFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    part.setUndecodedData(file.readAll());
    return true;
}

struct PartLoader
{
    QString fileName;

    PartLoader(const QString &path) : fileName(path) {}

    bool operator()(QMailMessagePart &part)
    {
        if ((part.referenceType() == QMailMessagePart::None) &&
            (part.multipartType() == QMailMessagePartContainer::MultipartNone)) {
            QString partFilePath;
            QString localContentFile = QUrl(part.contentLocation()).toLocalFile();
            bool localAttachment = QFile::exists(localContentFile) && !localContentFile.isEmpty() && !part.hasBody();
            if (localAttachment)
                partFilePath = QUrl(part.contentLocation()).toLocalFile();
            else
                partFilePath = QmfStorageManager::messagePartFilePath(part, fileName);

            if (QFile::exists(partFilePath)) {
                // Is the file content in encoded or decoded form?  Since we're delivering
                // server-side data, the parameter seems reversed...
                QMailMessageBody::EncodingStatus dataState(part.contentAvailable() ? QMailMessageBody::AlreadyEncoded : QMailMessageBody::RequiresEncoding);
                part.setBody(QMailMessageBody::fromFile(partFilePath, part.contentType(), part.transferEncoding(), dataState));
                if (!part.hasBody() && QFile(partFilePath).size())
                    return false;
            }
        }

        // Load undecoded data if any.
        loadPartUndecoded(part, fileName);

        return true;
    }
};

QMailStore::ErrorCode QmfStorageManager::load(const QString &identifier, QMailMessage *message)
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

    QMailMessage result(QMailMessage::fromSkeletonRfc2822File(path));

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

bool QmfStorageManager::init()
{
    // It used to be possible for accounts to not have a storage service configured.
    // If so, add a configuration to those accounts for QmfStorageManager
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
                svcCfg.setValue("version", "101");
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

    return true;
}

void QmfStorageManager::clearContent()
{
    // Delete all content files
    recursivelyRemovePath(messagesBodyPath(QMailAccountId()));

    // Recreate the default storage directory
    QString path(messagesBodyPath(QMailAccountId()));
    QDir dir(path);
    if (!dir.exists() && !dir.mkpath(path)) {
        qMailLog(Messaging) << "Unable to recreate messages storage directory " << path;
    }
}

const QString &QmfStorageManager::messagesBodyPath(const QMailAccountId &accountId)
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

QString QmfStorageManager::messageFilePath(const QString &fileName, const QMailAccountId &accountId)
{
    return messagesBodyPath(accountId) + '/' + fileName;
}

QString QmfStorageManager::messagePartFilePath(const QMailMessagePart &part, const QString &fileName)
{
    return messagePartDirectory(fileName) + '/' + part.location().toString(false); 
}

QString QmfStorageManager::messagePartUndecodedFilePath(const QMailMessagePart &part, const QString &fileName)
{
    return messagePartDirectory(fileName) + '/' + part.location().toString(false) + "-raw";
}

QString QmfStorageManager::messagePartDirectory(const QString &fileName)
{
    return fileName + "-parts";
}

static bool storePartUndecoded(const QMailMessagePart &part, const QString &fileName, QList< QSharedPointer<QFile> > *openParts)
{
    // We save the undecoded data contained in part, so they can be reused
    // later for signature checking for instance.
    QString partFilePath(QmfStorageManager::messagePartUndecodedFilePath(part, fileName));

    // We need to write the content to a new file
    QSharedPointer<QFile> file(new QFile(partFilePath));
    if (!file->open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open new message part content file:" << partFilePath;
        return false;
    }

    // Write the part content to file
    QByteArray undecodedData = part.undecodedData();
    if (file->write(undecodedData) != undecodedData.length()) {
        qMailLog(Messaging) << "Unable to save message part content, removing temporary file:" << partFilePath;
        file->close();
        if (!QFile::remove(partFilePath)){
            qWarning()  << "Unable to remove temporary message part content file:" << partFilePath;
        }
        return false;
    }

    if (openParts) {
        openParts->append(file);
    } else {
        syncFile(file);
    }
    return true;
}

struct PartStorer
{
    QMailMessage *message;
    QString fileName;
    QString existing;
    QList< QSharedPointer<QFile> > *openParts;
    bool allowRename;

    PartStorer(QMailMessage *m, const QString &f, const QString &e, QList< QSharedPointer<QFile> > *o, bool ar)
     : message(m), fileName(f), existing(e), openParts(o), allowRename(ar) {}

    bool operator()(const QMailMessagePart &part)
    {
        if ((part.referenceType() == QMailMessagePart::None) &&
            (part.multipartType() == QMailMessagePartContainer::MultipartNone) &&
            part.hasBody()) {
            // We need to store this part
            QString partFilePath(QmfStorageManager::messagePartFilePath(part, fileName));

            if (!part.contentModified() && !existing.isEmpty() && allowRename) {
                // This part is not modified; see if we can simply move the existing file to the new identifier
                QString existingPath(QmfStorageManager::messagePartFilePath(part, existing));
                if (QFile::rename(existingPath, partFilePath)) {
                    return true;
                }
            }

            // We can only write the content in decoded form if it is complete
            QMailMessageBody::EncodingFormat outputFormat(part.contentAvailable() ? QMailMessageBody::Decoded : QMailMessageBody::Encoded);

            QString detachedFile = message->customField("qmf-detached-part-filename");
            if (!detachedFile.isEmpty()) {
                // We can take ownership of the file if that helps
                if ((outputFormat == QMailMessageBody::Encoded) ||
                    // If the output should be decoded, then only unchanged 'encodings 'can be used directly
                    ((part.transferEncoding() != QMailMessageBody::Base64) &&
                     (part.transferEncoding() != QMailMessageBody::QuotedPrintable))) {
                    // Try to take ownership of the file
                    if (QFile::rename(detachedFile, partFilePath)) {
                        message->removeCustomField("qmf-detached-part-filename");
                        return true;
                    }
                }
            }

            // We need to write the content to a new file
            QSharedPointer<QFile> file(new QFile(partFilePath));
            if (!file->open(QIODevice::WriteOnly)) {
                qWarning() << "Unable to open new message part content file:" << partFilePath;
                return false;
            }

            // Write the part content to file
            QDataStream out(file.data());
            if (!part.body().toStream(out, outputFormat) || (out.status() != QDataStream::Ok)) {
                qMailLog(Messaging) << "Unable to save message part content, removing temporary file:" << partFilePath;
                file->close();
                if (!QFile::remove(partFilePath)){
                    qWarning()  << "Unable to remove temporary message part content file:" << partFilePath;
                }
                return false;
            }

            if (openParts) {
                openParts->append(file);
            } else {
                syncFile(file);
            }
        }
        if (!part.undecodedData().isEmpty() && !storePartUndecoded(part, fileName, openParts))
            return false;

        return true;
    }
};

bool QmfStorageManager::addOrRenameParts(QMailMessage *message, const QString &fileName, const QString &existing, QMailContentManager::DurabilityRequirement durability)
{
    // Ensure that the part directory exists
    QString partDirectory(messagePartDirectory(fileName));
    if (!QDir(partDirectory).exists()) {
        if (!QDir::root().mkpath(partDirectory)) {
            qMailLog(Messaging) << "Unable to create directory for message part content:" << partDirectory;
            return false;
        }
    }

    QList< QSharedPointer<QFile> > openParts;
    bool durable = (durability == QMailContentManager::EnsureDurability);
    bool allowRename = (durability != QMailContentManager::NoDurability);
    PartStorer partStorer(message, fileName, existing, (durable ? 0 : &openParts), allowRename);
    if (!const_cast<const QMailMessage*>(message)->foreachPart(partStorer)) {
        qMailLog(Messaging) << "Unable to store parts for message:" << fileName;
        return false;
    }

    if (durability == QMailContentManager::NoDurability)
      return true; // Don't sync parts, they should be sync'd later
    foreach(QSharedPointer<QFile> part, openParts) {
        syncLater(part);
    }

    return true;
}

bool QmfStorageManager::removeParts(const QString &fileName)
{
    bool result(true);

    QString partDirectory(messagePartDirectory(fileName));

    QDir dir(partDirectory);
    if (dir.exists()) {
        // Remove any files in this directory
        foreach (const QString &entry, dir.entryList()) {
            if ((entry != QString('.')) && (entry != QLatin1String(".."))) {
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

void QmfStorageManager::syncLater(QSharedPointer<QFile> file)
{
#ifdef USE_FSYNC_PER_FILE
    _openFiles.append(file);
#else
    // In the case of managing multiple files use ::sync, otherwise use fsync
    if (!_useFullSync) {
        if (_openFiles.count()) {
            // Multiple files case
            _useFullSync = true;
            _openFiles.clear();
        } else {
            _openFiles.append(file);
        }
    }
#endif
}

QmfStorageManagerPlugin::QmfStorageManagerPlugin()
    : QMailContentManagerPlugin()
{
}

QString QmfStorageManagerPlugin::key() const
{
    return gKey;
}

QMailContentManager *QmfStorageManagerPlugin::create()
{
    return new QmfStorageManager(this);
}
