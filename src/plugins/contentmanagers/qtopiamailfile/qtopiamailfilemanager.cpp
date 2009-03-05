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
#ifndef QMAIL_QTOPIA
#include <QtPlugin>
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
        removeParts(*message, message->contentIdentifier());

        return QMailStore::FrameworkFault;
    }

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
    path.setNameFilters(QStringList() << (fi.fileName() + '*'));

    foreach (const QString &entry, path.entryList()) {
        QString filePath(path.filePath(entry));
        if (!QFile::remove(filePath)) {
            qMailLog() << "Unable to remove content file:" << filePath;
            result = QMailStore::FrameworkFault;
        }
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
    return fileName + '-' + part.location().toString(false); 
}

bool QtopiamailfileManager::addOrRenameParts(QMailMessage *message, const QMailMessagePartContainer &container, const QString &fileName, const QString &existing)
{
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

                    if (additionRequired) {
                        QString detachedFile = message->customField("qtopiamail-detached-filename");
                        if (!detachedFile.isEmpty()) {
                            // Try to take ownership of the file
                            if (QFile::rename(detachedFile, partFilePath)) {
                                message->removeCustomField("qtopiamail-detached-filename");
                            }
                        } else {
                            QFile file(partFilePath);
                            if (!file.open(QIODevice::WriteOnly)) {
                                qMailLog(Messaging) << "Unable to open new message part content file:" << partFilePath;
                                return false;
                            }

                            // Write the part content to file
                            QDataStream out(&file);
                            if (!part.body().toStream(out, QMailMessageBody::Decoded) || (out.status() != QDataStream::Ok)) {
                                qMailLog(Messaging) << "Unable to save message part content, removing temporary file:" << partFilePath;
                                if (!QFile::remove(partFilePath)){
                                    qMailLog(Messaging) << "Unable to remove temporary message part content file:" << partFilePath;
                                }

                                return false;
                            }
                        }
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
                QString partFilePath(messagePartFilePath(part, fileName));
                if (QFile::exists(partFilePath)) {
                    part.setBody(QMailMessageBody::fromFile(partFilePath, part.contentType(), part.transferEncoding(), QMailMessageBody::RequiresEncoding));
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

void QtopiamailfileManager::removeParts(const QMailMessagePartContainer &container, const QString &fileName)
{
    for (uint i = 0; i < container.partCount(); ++i) {
        const QMailMessagePart &part(container.partAt(i));

        if (part.multipartType() == QMailMessagePartContainer::MultipartNone) {
            if (part.hasBody()) {
                QString partFilePath(messagePartFilePath(part, fileName));

                if (QFile::exists(partFilePath)) {
                    if (!QFile::remove(partFilePath)){
                        qMailLog(Messaging) << "Unable to remove message part content file:" << partFilePath;
                    }
                }
            }
        } else {
            // Remove any sub-parts of this part
            removeParts(part, fileName);
        }
    }
}

#ifdef QMAIL_QTOPIA
QTOPIA_EXPORT_PLUGIN( QtopiamailfileManagerPlugin )
#else
Q_EXPORT_PLUGIN2(qtopiamailfilemanager,QtopiamailfileManagerPlugin)
#endif

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

