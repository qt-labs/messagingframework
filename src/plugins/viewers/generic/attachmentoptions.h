/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef ATTACHMENTOPTIONS_H
#define ATTACHMENTOPTIONS_H

#include <QByteArray>
#include <QDialog>
#include <QList>
#include <QSize>
#include <QString>

class QByteArray;
class QLabel;
class QMailMessagePart;
class QPushButton;
class QString;

class AttachmentOptions : public QDialog
{
    Q_OBJECT

public:
    enum ContentClass
    {
        Text,
        Image,
        Media,
        Multipart,
        Other
    };

    AttachmentOptions(QWidget* parent);
    ~AttachmentOptions();

    QSize sizeHint() const;

signals:
    void retrieve(const QMailMessagePart& part);

public slots:
    void setAttachment(const QMailMessagePart& part);

    void viewAttachment();
    void saveAttachment();
    void retrieveAttachment();

private:
    QSize _parentSize;
    QLabel* _name;
    QLabel* _type;
    //QLabel* _comment;
    QLabel* _sizeLabel;
    QLabel* _size;
    QPushButton* _view;
    QLabel* _viewer;
    QPushButton* _save;
    QLabel* _document;
    QPushButton* _retrieve;
    const QMailMessagePart* _part;
    ContentClass _class;
    QString _decodedText;
    QByteArray _decodedData;
    QStringList _temporaries;
};

#endif

