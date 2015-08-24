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


#include "attachmentoptions.h"
#include "browserwidget.h"
#include "genericviewer.h"
#include "qmailaccount.h"
#include "qmailmessage.h"

#include <QAction>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <QFormLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QString>
#include <QStyle>
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QVBoxLayout>


class TextDisplay : public QDialog
{
    Q_OBJECT

public:
    TextDisplay(QWidget* parent);
    ~TextDisplay();

    void setText(const QString& text, const QString& subType);

public slots:
    void toggleLineWrapMode();

private:
    QTextBrowser* _browser;
    QTextEdit::LineWrapMode _mode;
};

TextDisplay::TextDisplay(QWidget* parent)
    : QDialog(parent),
      _browser(new QTextBrowser(this)),
      _mode(QTextEdit::WidgetWidth)
{
    _browser->setLineWrapMode(_mode);

    QVBoxLayout* vb = new QVBoxLayout(this);
    vb->addWidget(_browser);

    QAction* toggleLineWrap = new QAction(tr("Wrap text"), this);
    toggleLineWrap->setCheckable(true);
    toggleLineWrap->setChecked(true);
    toggleLineWrap->setVisible(true);

    connect(toggleLineWrap, SIGNAL(triggered()), this, SLOT(toggleLineWrapMode()));
    addAction(toggleLineWrap);

    setContextMenuPolicy(Qt::ActionsContextMenu);
}

TextDisplay::~TextDisplay()
{
}

void TextDisplay::setText(const QString& text, const QString& subType)
{
    if (subType.toLower() == "html") {
        _browser->setHtml(text);
    } else {
        _browser->setPlainText(text);
    }
}

void TextDisplay::toggleLineWrapMode()
{
    _mode = (_mode == QTextEdit::NoWrap ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
    _browser->setLineWrapMode(_mode);
}


class ImageDisplay : public QDialog
{
    Q_OBJECT

public:
    ImageDisplay(QWidget* parent);
    ~ImageDisplay();

    void setImage(const QByteArray& imageData);

    QSize sizeHint() const;

public slots:
    void sizeToFit();
    void sizeDefault();
    void sizeActual();
    void zoomToFit();

private:
    void setImage(const QImage& image);
    void loadImage(QSize size, Qt::AspectRatioMode mode, bool increase = false);

    QSize _size;
    QScrollArea* _area;
    QAction* _sizeToFit;
    QAction* _sizeDefault;
    QAction* _sizeActual;
    QAction* _zoomToFit;
    QByteArray _imageData;
    QSize _imageSize;
};

ImageDisplay::ImageDisplay(QWidget* parent)
    : QDialog(parent),
      _size(parent->size()),
      _area(new QScrollArea(this)), 
      _sizeToFit(new QAction(tr("Size to fit"), this)),
      _sizeDefault(new QAction(tr("Default size"), this)),
      _sizeActual(new QAction(tr("Actual size"), this)),
      _zoomToFit(new QAction(tr("Zoom to fit"), this))
{
    _area->setWidgetResizable(true);
    _area->setFrameStyle(QFrame::NoFrame);

    QVBoxLayout* vb = new QVBoxLayout(this);
    vb->addWidget(_area);
    vb->setMargin(0);
    vb->setSpacing(0);

    connect(_sizeToFit, SIGNAL(triggered()), this, SLOT(sizeToFit()));
    _sizeToFit->setVisible(true);
    addAction(_sizeToFit);

    connect(_sizeDefault, SIGNAL(triggered()), this, SLOT(sizeDefault()));
    _sizeDefault->setVisible(false);
    addAction(_sizeDefault);

    connect(_sizeActual, SIGNAL(triggered()), this, SLOT(sizeActual()));
    _sizeActual->setVisible(true);
    addAction(_sizeActual);

    connect(_zoomToFit, SIGNAL(triggered()), this, SLOT(zoomToFit()));
    _zoomToFit->setVisible(false);
    addAction(_zoomToFit);

    setContextMenuPolicy(Qt::ActionsContextMenu);
}

ImageDisplay::~ImageDisplay()
{
}

QSize ImageDisplay::sizeHint() const
{
    return _size;
}

void ImageDisplay::setImage(const QImage& image)
{
    QLabel* label = new QLabel();
    label->setFrameStyle(QFrame::NoFrame);
    label->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    label->setAlignment(Qt::AlignCenter);
    label->setPixmap(QPixmap::fromImage(image));

    _area->setWidget(label);
}

void ImageDisplay::setImage(const QByteArray& imageData)
{
    _imageData = imageData;
    _imageSize = QSize();

    sizeDefault();
}

void ImageDisplay::sizeDefault()
{
    // Max size should be bounded by our display window, which will possibly
    // have a horizontal or vertical scrollbar
    int overhead = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    loadImage(QSize(_size.width() - overhead, _size.height() - overhead), Qt::KeepAspectRatioByExpanding);

    _sizeDefault->setVisible(false);
}

void ImageDisplay::sizeToFit()
{
    loadImage(_size, Qt::KeepAspectRatio);

    _sizeToFit->setVisible(false);
    _sizeDefault->setVisible(true);
}

void ImageDisplay::sizeActual()
{
    loadImage(_imageSize, Qt::KeepAspectRatio);

    _sizeActual->setVisible(false);
    _sizeDefault->setVisible(true);
}

void ImageDisplay::zoomToFit()
{
    loadImage(_size, Qt::KeepAspectRatio, true);

    _zoomToFit->setVisible(false);
    _sizeDefault->setVisible(true);
}

void ImageDisplay::loadImage(QSize size, Qt::AspectRatioMode mode, bool increase)
{
    // Create an image from the input data
    QDataStream imageStream(&_imageData, QIODevice::ReadOnly);
    QImageReader imageReader(imageStream.device());

    if (imageReader.supportsOption(QImageIOHandler::Size)) {
        _imageSize = imageReader.size();

        // See if the image needs to be scaled during load
        if (increase || ((_imageSize.width() > size.width()) || (_imageSize.height() > size.height())))
        {
            QSize displaySize(_imageSize);

            // And the loaded size should maintain the image aspect ratio
            displaySize.scale(size, mode);
            imageReader.setQuality( 49 ); // Otherwise Qt smooth scales
            imageReader.setScaledSize(displaySize);
        }
    }

    QImage image = imageReader.read();

    if (!imageReader.supportsOption(QImageIOHandler::Size)) {
        _imageSize = image.size();

        // We need to scale it now
        if (increase || ((_imageSize.width() > size.width()) || (_imageSize.height() > size.height())))
            image = image.scaled(size, mode);
    }

    bool largeImage = (_imageSize.width() > _size.width()) || (_imageSize.height() > _size.height());
    _sizeToFit->setVisible(largeImage);
    _sizeActual->setVisible(largeImage);
    _zoomToFit->setVisible(!largeImage);

    setImage(image);
}


AttachmentOptions::AttachmentOptions(QWidget* parent)
    : QDialog(parent),
      _parentSize(parent->size()),
      _name(new QLabel()),
      _type(new QLabel()),
      //_comment(new QLabel()),
      _sizeLabel(new QLabel(tr("Size"))),
      _size(new QLabel()),
      _view(new QPushButton()),
      _viewer(new QLabel()),
      _save(new QPushButton()),
      _document(new QLabel()),
      _retrieve(new QPushButton()),
      _forward(new QPushButton()),
      _part(0),
      _class(Other)
{
    setWindowTitle(tr("Attachment"));

    QFormLayout* layout = new QFormLayout(this);

    _name->setWordWrap(true);
    layout->addRow(tr("Name"), _name);

    _type->setWordWrap(true);
    layout->addRow(tr("Type"), _type);

    //_comment->setWordWrap(true);
    //layout->addWidget(tr("Comment"), _comment);

    _size->setWordWrap(true);
    layout->addRow(_sizeLabel, _size);

    QVBoxLayout* vb = new QVBoxLayout();

    connect(_view, SIGNAL(clicked()), this, SLOT(viewAttachment()));
    vb->addWidget(_view);

    vb->addWidget(_viewer);

    _save->setText(tr("Add to documents"));
    connect(_save, SIGNAL(clicked()), this, SLOT(saveAttachment()));
    vb->addWidget(_save);

    _document->setText("<i><small><center>" + tr("Already added to Documents") + "</center></small></i>");
    vb->addWidget(_document);

    _retrieve->setText(tr("Download attachment"));
    connect(_retrieve, SIGNAL(clicked()), this, SLOT(retrieveAttachment()));
    vb->addWidget(_retrieve);

    _forward->setText(tr("Forward attachment"));
    connect(_forward, SIGNAL(clicked()), this, SLOT(forwardAttachment()));
    vb->addWidget(_forward);

    layout->addRow(vb);
}

AttachmentOptions::~AttachmentOptions()
{
    while (!_temporaries.isEmpty()) {
        QString fileName(_temporaries.takeFirst());
        if (QFile::exists(fileName)) {
            QFile::remove(fileName);
        }
    }

}

QSize AttachmentOptions::sizeHint() const
{
    return _parentSize;
}

static QString humanReadable(int size)
{
    if(size < 1024)
        return QObject::tr("%n byte(s)", "", size);
    else if(size < (1024 * 1024))
        return QObject::tr("%1 KB").arg(((float)size)/1024.0, 0, 'f', 1);
    else if(size < (1024 * 1024 * 1024))
        return QObject::tr("%1 MB").arg(((float)size)/(1024.0 * 1024.0), 0, 'f', 1);
    else
        return QObject::tr("%1 GB").arg(((float)size)/(1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

typedef QMap<QString, AttachmentOptions::ContentClass> ClassMap;

static ClassMap initMap()
{
    ClassMap map;
    map["text"] = AttachmentOptions::Text;
    map["image"] = AttachmentOptions::Image;
    map["audio"] = AttachmentOptions::Media;
    map["video"] = AttachmentOptions::Media;
    map["multipart"] = AttachmentOptions::Multipart;
    return map;
}

static AttachmentOptions::ContentClass contentClass(const QMailMessageContentType& type)
{
    static ClassMap map(initMap());

    ClassMap::const_iterator it = map.find(type.type().toLower());
    AttachmentOptions::ContentClass contentClass(it != map.end() ? it.value() : AttachmentOptions::Other);

    // Test for exceptions that we can't handle internally
    if (contentClass == AttachmentOptions::Text) {
        if (type.subType().toLower() == "x-vcard") {
            // Have the content system deal with this
            contentClass = AttachmentOptions::Other;
        }
    }

    return contentClass;
}

void AttachmentOptions::setAttachment(const QMailMessagePart& msgPart)
{
    _part = &msgPart;
    _class = contentClass(_part->contentType());

    bool isDocument = false;
    bool isDeleted = false;
    bool isAvailable = _part->partialContentAvailable();
    bool isRetrievable = !_part->contentAvailable();
    int size = 0;
    QString sizeText;

    if (isAvailable) {
        size = _part->body().length();
        sizeText = humanReadable(size);
        if (isRetrievable)
            sizeText.append(tr(" of ") + humanReadable(_part->contentDisposition().size()));
    } else {
        // This part is not yet available
        _document->setText("<i><small><center>" + tr("Document not yet retrieved") + "</center></small></i>");
        size = _part->contentDisposition().size();
        sizeText = humanReadable(size);
    }

    QString typeName = _part->contentType().content();

    _name->setText(_part->displayName());
    _type->setText(typeName);
    _size->setText(sizeText);

    _viewer->setVisible(false);
    _view->setVisible(false);

    if (isAvailable && !isDeleted) {
        if (_class == Media) {
            _view->setText(tr("Play"));
            _view->setVisible(true);
        } else if (_class == Text || _class == Image) {
            _view->setText(tr("View"));
            _view->setVisible(true);
        } else {
#if 0
            // See if there is a viewer available for this type
            QMimeType mt(_part->contentType().content());
            if (!mt.id().isEmpty() && !QMimeType::applicationsFor(mt).isEmpty()) {
#endif
                _view->setText(tr("View"));
                _view->setVisible(true);
#if 0
            } else {
                _viewer->setText("<i><small><center>" + tr("No viewer available") + "</center></small></i>");
                _viewer->setVisible(true);
            }
#endif
        }
    }

    _sizeLabel->setVisible(!isDeleted && (size > 0));
    _size->setVisible(!isDeleted && (size > 0));
    _save->setVisible(!isRetrievable && !isDocument && (size > 0));
    _retrieve->setVisible(isRetrievable);
    _document->setVisible(isDocument);

    if (isRetrievable) {
        // We can't forward this part unless the account supports transmission by reference 
        QMailMessageMetaData metaData(_part->location().containingMessageId());
        QMailAccount originAccount(metaData.parentAccountId());

        _forward->setVisible(originAccount.status() & QMailAccount::CanReferenceExternalData);
    } else {
        _forward->setVisible(true);
    }
}

void AttachmentOptions::viewAttachment()
{
    if (_class == Text || _class == Image) {
        // We can display this data directly without a helper app
        if (_class == Text) {
            if (_decodedText.isNull())
                _decodedText = _part->body().data();

            TextDisplay display(this);
            display.setText(_decodedText, _part->contentType().subType());
            display.exec();
        } else {
            if (_decodedData.isNull())
                _decodedData = _part->body().data(QMailMessageBody::Decoded);

            ImageDisplay display(this);
            display.setImage(_decodedData);
            display.exec();
        }
    } else if (_part->contentType().content().toLower() == "message/rfc822") {
        QDialog display(this);
        QGridLayout *gl = new QGridLayout(&display);
        GenericViewer *viewer = new GenericViewer(&display);
        gl->addWidget(viewer->widget(), 0, 0);
        QMailMessage message(QMailMessage::fromRfc2822(_part->body().data().toLatin1()));
        viewer->setMessage(message);
        display.exec();
    } else {
#if 1
        qWarning() << "Cannot display message of type:" << _part->contentType().content();
#else
        QMimeType mt(_part->contentType().content());
        if (!mt.id().isEmpty()) {
            if (_decodedData.isNull())
                _decodedData = _part->body().data(QMailMessageBody::Decoded);

            // Write the data to a temporary file
            QString templateText(QMail::tempPath() + "genericviewer-XXXXXX");
            if (!mt.extensions().isEmpty()) {
                templateText.append('.').append(mt.extensions().last());
            } else {
                // Try to get an extension from the part name
                QString extension;
                QString filename = _part->contentDisposition().filename();
                int index = -1;
                if ((index = filename.lastIndexOf('.')) != -1) {
                    extension = filename.mid(index + 1);
                } else {
                    filename = _part->contentType().name();
                    if ((index = filename.lastIndexOf('.')) != -1) {
                        extension = filename.mid(index + 1);
                    }
                }

                if (!extension.isEmpty()) {
                    templateText.append('.').append(extension);
                }
            }

            QTemporaryFile* tempFile = new QTemporaryFile();
            tempFile->setFileTemplate(templateText);
            if (tempFile->open()) {
                tempFile->write(_decodedData);

                // We need to invoke the associated app
                QContent content(tempFile->fileName());
                
                // Ensure the associated app doesn't create a document from this temporary file
                content.setType(mt.id());
                content.setName(_part->displayName());
                content.setRole(QContent::Data);
                content.commit();

                _temporaries.append(content);
                tempFile->setAutoRemove(false);

                if (!content.executableName().isNull()) {
                    content.execute();
                } else {
                    QMessageBox mb(_view->text() + ' ' + tr("Error"),
                                   tr("No application associated with file of type") + ' ' + mt.id(),
                                   QMessageBox::Warning,
                                   QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
                    mb.exec();
                }
            } else {
                QMessageBox mb(_view->text() + ' ' + tr("Error"),
                               tr("Unable to create temporary file"),
                               QMessageBox::Warning,
                               QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
                mb.exec();
            }

            delete tempFile;
        } else {
            QMessageBox mb(_view->text() + ' ' + tr("Error"),
                           tr("Unknown file type") + ": " + _part->contentType().content(),
                           QMessageBox::Warning,
                           QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
            mb.exec();
        }
#endif
    }
}

void AttachmentOptions::saveAttachment()
{
    QString fileName(_part->writeBodyTo(QDir::currentPath()));
    if (!fileName.isEmpty()) {
#if 0
        // Create a document object from the saved file
        QContent document(fileName);

        if (_part->hasBody()) {
            QMailMessageContentType type(_part->contentType());

            if (document.drmState() == QContent::Unprotected)
                document.setType(_part->contentType().content());
        }

        document.setName(_part->displayName());
        document.setRole(QContent::Document);
        document.commit();
#endif

        _document->setText("<i><small><center>" + tr("Saved in:") + QDir::currentPath() + "</center></small></i>");
        _document->setVisible(true);
        _save->setVisible(false);
    } else {
        QMessageBox mb(tr("Unable to save attachment"),
                       tr("Please ensure that there is space available for Documents"),
                       QMessageBox::Warning,
                       QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
        mb.exec();
    }
}

void AttachmentOptions::retrieveAttachment()
{
    if ((_class == Text) && (_part->contentType().subType().toLower() == "plain")) {
        // Retrieve text/plain progressively
        emit retrievePortion(*_part, 5 * 1024);
    } else {
        emit retrieve(*_part);
    }

    accept();
}

void AttachmentOptions::forwardAttachment()
{
    emit respondToPart(_part->location(), QMailMessage::ForwardPart);

    accept();
}

#include "attachmentoptions.moc"
