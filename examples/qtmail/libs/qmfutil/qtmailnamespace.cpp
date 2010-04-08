#include "qtmailnamespace.h"
#include <QMap>
#include <QDebug>

typedef QMap<QString,QIcon> IconMap;

static QIcon loadIcon(const QString& themeIdentifier, const QString& resourceFallback)
{
#if ((QT_VERSION < QT_VERSION_CHECK(4, 6, 0)) || !defined(Q_OS_LINUX))
    Q_UNUSED(themeIdentifier);
    return QIcon(resourceFallback);
#else
    return QIcon::fromTheme(themeIdentifier,QIcon(resourceFallback));
#endif
}

static IconMap init()
{
    IconMap m;
    m.insert("clear",loadIcon("edit-clear",":icon/erase"));
    m.insert("attachment",loadIcon("mail-attachment",":icon/attach"));
    m.insert("add",loadIcon("list-add",":icon/add"));
    m.insert("remove",loadIcon("list-remove",":icon/erase"));
    m.insert("reset", loadIcon("document-revert", ":icon/reset"));
    m.insert("close",loadIcon("window-close",":icon/close"));
    m.insert("uparrow",loadIcon("up",":icon/up"));
    m.insert("downarrow",loadIcon("down",":icon/down"));
    m.insert("sendandreceive",loadIcon("mail-send-receive",":icon/sync"));
    m.insert("cancel",loadIcon("process-stop",":icon/cancel"));
    m.insert("settings",loadIcon("package_settings",":icon/settings"));
    m.insert("compose",loadIcon("mail-message-new",":icon/new"));
    m.insert("search",loadIcon("search",":icon/find"));
    m.insert("reply",loadIcon("mail-reply-sender",":icon/reply"));
    m.insert("replyall",loadIcon("mail-reply-all",":icon/replyall"));
    m.insert("forward",loadIcon("mail-forward",":icon/forward"));
    m.insert("deletemail",loadIcon("edit-delete-mail",":icon/trash"));
    m.insert("folder",loadIcon("folder",":icon/folder"));
    m.insert("folderremote",loadIcon("folder-remote",":icon/folder-remote"));
    m.insert("inboxfolder",loadIcon("inboxfolder",":icon/inbox"));
    m.insert("trashfolder",loadIcon("emptytrash",":icon/trash"));
    m.insert("junkfolder",loadIcon("mail-mark-junk",":icon/folder"));
    m.insert("sentfolder",loadIcon("mail-send",":icon/sent"));
    m.insert("accountfolder",loadIcon("accountfolder",":icon/account"));
    m.insert("outboxfolder",loadIcon("outboxfolder",":icon/outbox"));
    m.insert("draftfolder",loadIcon("emblem-draft",":icon/drafts"));
    m.insert("quit",loadIcon("exit",":icon/quit"));
    return m;
}

QIcon Qtmail::icon(const QString& name)
{
    static IconMap icons(init());
    QIcon result = icons[name];

    if(result.isNull())
        qWarning() << name << " icon not found.";

    return icons[name];
}
