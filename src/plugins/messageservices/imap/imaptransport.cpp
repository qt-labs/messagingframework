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

#include "imaptransport.h"
#include <qmaillog.h>
#include <qbuffer.h>
#include <qdatastream.h>

#ifdef QT_QMF_HAVE_ZLIB
#include <zlib.h>

#undef compress // defined in Qt's zconf.h and clashing with ImapTransport::transport()

/* From RFC4978 The IMAP COMPRESS:   
   "When using the zlib library (see [RFC1951]), the functions
   deflateInit2(), deflate(), inflateInit2(), and inflate() suffice to
   implement this extension.  The windowBits value must be in the range
   -8 to -15, or else deflateInit2() uses the wrong format.
   deflateParams() can be used to improve compression rate and resource
   use.  The Z_FULL_FLUSH argument to deflate() can be used to clear the
   dictionary (the receiving peer does not need to do anything)."
   
   Total zlib mem use is 176KB plus a 'few kilobytes' per connection that uses COMPRESS:
   96KB for deflate, 24KB for 3x8KB buffers, 32KB plus a 'few' kilobytes for inflate.
*/

class Rfc1951Compressor
{
public:
    Rfc1951Compressor(int chunkSize = 8192);
    ~Rfc1951Compressor();

    bool write(QDataStream *out, QByteArray *in);

private:
    int _chunkSize;
    z_stream _zStream;
    char *_buffer;
};

Rfc1951Compressor::Rfc1951Compressor(int chunkSize)
{
    _chunkSize = chunkSize;
    _buffer = new char[chunkSize];

    /* allocate deflate state */
    _zStream.zalloc = Z_NULL;
    _zStream.zfree = Z_NULL;
    _zStream.opaque = Z_NULL;

    bool ok(deflateInit2(&_zStream,
                          Z_DEFAULT_COMPRESSION, 
                          Z_DEFLATED, 
                          -(MAX_WBITS-2), // 32KB // MAX_WBITS == 15 (zconf.h) MEM128KB
                          MAX_MEM_LEVEL-2 , // 64KB // MAX_MEM_LEVEL = 9 (zconf.h) MEM256KB
                          Z_DEFAULT_STRATEGY) == Z_OK);
    Q_ASSERT(ok);
}

Rfc1951Compressor::~Rfc1951Compressor()
{
    delete _buffer;
    deflateEnd(&_zStream);
}

bool Rfc1951Compressor::write(QDataStream *out, QByteArray *in)
{
    _zStream.next_in = reinterpret_cast<Bytef*>(in->data());
    _zStream.avail_in = in->size();
    
    do {
        _zStream.next_out = reinterpret_cast<Bytef*>(_buffer);
        _zStream.avail_out = _chunkSize;
        int result = deflate(&_zStream, Z_SYNC_FLUSH);
        if (result != Z_OK &&
            result != Z_STREAM_END &&
            result != Z_BUF_ERROR) {
            return false;
        }
        out->writeRawData(_buffer, _chunkSize - _zStream.avail_out);
    } while (!_zStream.avail_out);
    return true;
}

class Rfc1951Decompressor
{
public:
    Rfc1951Decompressor(int chunkSize = 8192);
    ~Rfc1951Decompressor();

    bool consume(QIODevice *in);
    bool canReadLine() const;
    bool bytesAvailable() const;
    QByteArray readLine();
    QByteArray readAll();

private:
    int _chunkSize;
    z_stream _zStream;
    QByteArray _inBuffer;
    char *_stagingBuffer;
    QByteArray _output;
};

Rfc1951Decompressor::Rfc1951Decompressor(int chunkSize)
{
    _chunkSize = chunkSize;
    _stagingBuffer = new char[_chunkSize];

    /* allocate inflate state */
    _zStream.zalloc = Z_NULL;
    _zStream.zfree = Z_NULL;
    _zStream.opaque = Z_NULL;
    _zStream.avail_in = 0;
    _zStream.next_in = Z_NULL;
    bool ok(inflateInit2(&_zStream, -MAX_WBITS) == Z_OK);
    Q_ASSERT(ok);
}

Rfc1951Decompressor::~Rfc1951Decompressor()
{
    inflateEnd(&_zStream);
    delete _stagingBuffer;
}

bool Rfc1951Decompressor::consume(QIODevice *in)
{
    while (in->bytesAvailable()) {
        _inBuffer = in->read(_chunkSize);
        _zStream.next_in = reinterpret_cast<Bytef*>(_inBuffer.data());
        _zStream.avail_in = _inBuffer.size();
        do {
            _zStream.next_out = reinterpret_cast<Bytef *>(_stagingBuffer);
            _zStream.avail_out = _chunkSize;
            int result = inflate(&_zStream, Z_SYNC_FLUSH);
            if (result != Z_OK &&
                result != Z_STREAM_END &&
                result != Z_BUF_ERROR) {
                return false;
            }
            _output.append(_stagingBuffer, _chunkSize - _zStream.avail_out);
        } while (_zStream.avail_out == 0);
    }
    return true;
}

bool Rfc1951Decompressor::canReadLine() const
{
    return _output.contains('\n');
}

bool Rfc1951Decompressor::bytesAvailable() const
{
    return !_output.isEmpty();
}

QByteArray Rfc1951Decompressor::readLine()
{
    int eolPos = _output.indexOf('\n');
    if (eolPos == -1) {
        return QByteArray();
    }
    
    QByteArray result = _output.left(eolPos + 1);
    _output = _output.mid(eolPos + 1);
    return result;
}

QByteArray Rfc1951Decompressor::readAll()
{
    QByteArray result =  _output;
    _output.clear();
    return result;
}
#else
class Rfc1951Compressor
{
public:
    Rfc1951Compressor() {}
    ~Rfc1951Compressor() {}

    bool write(QDataStream *, QByteArray *) { return true;}
};

class Rfc1951Decompressor
{
public:
    Rfc1951Decompressor() {}
    ~Rfc1951Decompressor() {}

    bool consume(QIODevice *) { return true; }
    bool canReadLine() const { return true; }
    bool bytesAvailable() const { return true; }
    QByteArray readLine() { return QByteArray(); }
    QByteArray readAll() { return QByteArray(); }
};
#endif


ImapTransport::ImapTransport(const char* name)
    :QMailTransport(name),
     _compress(false),
     _decompressor(0),
     _compressor(0)
{
    test();
}

ImapTransport::~ImapTransport()
{
    delete _decompressor;
    delete _compressor;
}

bool ImapTransport::imapCanReadLine()
{
    if (!compress()) {
        return canReadLine();
    } else {
        _decompressor->consume(&socket());
        return _decompressor->canReadLine();
    }
}

bool ImapTransport::imapBytesAvailable()
{
    if (!compress()) {
        return bytesAvailable();
    } else {
        return _decompressor->bytesAvailable();
    }
}

QByteArray ImapTransport::imapReadLine()
{
    if (!compress()) {
        return readLine();
    } else {
        return _decompressor->readLine();
    }
}

QByteArray ImapTransport::imapReadAll()
{
    if (!compress()) {
        return readAll();
    } else {
        return _decompressor->readAll();
    }
}

bool ImapTransport::imapWrite(QByteArray *in)
{
    if (!compress()) {
        stream().writeRawData(in->constData(), in->length());
        return true;
    } else {
        return _compressor->write(&stream(), in);
    }
}

void ImapTransport::setCompress(bool comp)
{
    _compress = comp;
    if (comp) {
        if (!_compressor) {
            _compressor = new Rfc1951Compressor();
        }
        if (!_decompressor) {
            _decompressor = new Rfc1951Decompressor();
        }
    }
}

bool ImapTransport::compress()
{
    return _compress;
}

void ImapTransport::imapClose()
{
    close();
    _compress = false;
    delete _decompressor;
    _decompressor = 0;
    delete _compressor;
    _compressor = 0;
}

#ifndef QT_NO_SSL
bool ImapTransport::ignoreCertificateErrors(const QList<QSslError>& errors)
{
    QMailTransport::ignoreCertificateErrors(errors);

    // Because we can't ask the user (due to string freeze), let's default
    // to ignoring these errors...
    foreach (const QSslError& error, errors)
        if (error.error() == QSslError::NoSslSupport)
            return false;

    return true;
}
#endif

void ImapTransport::test()
{
#if 0
    qMailLog(IMAP) << "Rfc1951Compressor and Rfc1951Decompressor functional testing running...";
    // Mainly aiming to test for bounday conditions
    // So make the compression/decompression buffers about the same size as the input/output
    QByteArray data("This\n is some test data.\r\n The quick brown fox jumps over the lazy dog. 0123456789.\r\n");
    for(int i = 10; i <= 100; ++ i) {
        for(int j = 10; i <= 100; ++ i) {
            for (int k = 10; k <= 100; ++k) {
                Rfc1951Compressor compressor(i);
                Rfc1951Decompressor decompressor(j);
                QByteArray input(data.left(k));
                input += "\r\n";
                QByteArray compressed;
                {
                    QDataStream stream(&compressed, QIODevice::WriteOnly);
                    compressor.write(&stream, &input);
                }
                {
                    QByteArray output;
                    QBuffer buffer(&compressed);
                    buffer.open(QIODevice::ReadOnly);
                    decompressor.consume(&buffer);
                    while (decompressor.canReadLine()) {
                        output += decompressor.readLine();
                    }
                    if (input != output) {
                        qMailLog(IMAP) << "Test failure: input" << input.toHex() << "output" << output.toHex();
                        Q_ASSERT(input == output);
                    }
                }
            }
        }
    }
    qMailLog(IMAP) << "Rfc1951Compressor and Rfc1951Decompressor functional testing completed OK";
#endif
}
