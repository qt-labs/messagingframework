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

#ifndef QMAILCODEC_H
#define QMAILCODEC_H

#include "qmailglobal.h"

#include <QByteArray>
#include <QDataStream>
#include <QString>
#include <QTextStream>

class QTextCodec;

class QMF_EXPORT QMailCodec
{
public:
    enum { ChunkCharacters = 8192 };

    virtual ~QMailCodec();

    virtual QString name() const = 0;

    // Stream conversion interface including character translation
    virtual void encode(QDataStream& out, QTextStream& in, const QByteArray& charset = "UTF-8");
    virtual void decode(QTextStream& out, QDataStream& in, const QByteArray& charset);

    // Stream conversion interface
    virtual void encode(QDataStream& out, QDataStream& in);
    virtual void decode(QDataStream& out, QDataStream& in);

    // Convenience functions to encapsulate stream processing
    QByteArray encode(const QString& in, const QByteArray& charset = "UTF-8");
    QString decode(const QByteArray& in, const QByteArray& charset);

    QByteArray encode(const QByteArray& in);
    QByteArray decode(const QByteArray& in);

    // Static helper functions
    static QTextCodec * codecForName(const QByteArray& charset, bool translateAscii = true);
    static QByteArray bestCompatibleCharset(const QByteArray& charset, bool translateAscii);
    static void copy(QDataStream& out, QDataStream& in);
    static void copy(QTextStream& out, QDataStream& in, const QByteArray& charset = "UTF-8");
    static QString autoDetectEncoding(const QByteArray& text);
    static QString encodeModifiedUtf7(const QString &text);
    static QString decodeModifiedUtf7(const QString &text);

protected:
    // Helper functions to convert stream chunks
    virtual void encodeChunk(QDataStream& out, const unsigned char* in, int length, bool finalChunk) = 0;
    virtual void decodeChunk(QDataStream& out, const char* in, int length, bool finalChunk) = 0;
};

class QMF_EXPORT QMailBase64Codec : public QMailCodec
{
public:
    enum ContentType { Text, Binary };

    QMailBase64Codec(ContentType content, int maximumLineLength = -1);

    QString name() const override;

protected:
    void encodeChunk(QDataStream& out, const unsigned char* in, int length, bool finalChunk) override;
    void decodeChunk(QDataStream& out, const char* in, int length, bool finalChunk) override;

private:
    ContentType _content;
    int _maximumLineLength;

    unsigned char _encodeBuffer[3];
    unsigned char* _encodeBufferOut;
    int _encodeLineCharsRemaining;

    unsigned char _decodeBuffer[4];
    unsigned char* _decodeBufferOut;
    int _decodePaddingCount;

    unsigned char _lastChar;
};

class QMF_EXPORT QMailQuotedPrintableCodec : public QMailCodec
{
public:
    enum ContentType { Text, Binary };
    enum ConformanceType { Rfc2045, Rfc2047 };

    QMailQuotedPrintableCodec(ContentType content, ConformanceType conformance, int maximumLineLength = -1);

    QString name() const override;

protected:
    void encodeChunk(QDataStream& out, const unsigned char* in, int length, bool finalChunk) override;
    void decodeChunk(QDataStream& out, const char* in, int length, bool finalChunk) override;

private:
    ContentType _content;
    ConformanceType _conformance;
    int _maximumLineLength;

    int _encodeLineCharsRemaining;
    unsigned char _encodeLastChar;

    char _decodePrecedingInput;
    unsigned char _decodeLastChar;
};

class QMF_EXPORT QMailPassThroughCodec : public QMailCodec
{
    QString name() const override;

protected:
    void encodeChunk(QDataStream& out, const unsigned char* in, int length, bool finalChunk) override;
    void decodeChunk(QDataStream& out, const char* in, int length, bool finalChunk) override;
};

class QMF_EXPORT QMailLineEndingCodec : public QMailCodec
{
    QString name() const override;

public:
    QMailLineEndingCodec();

protected:
    void encodeChunk(QDataStream& out, const unsigned char* in, int length, bool finalChunk) override;
    void decodeChunk(QDataStream& out, const char* in, int length, bool finalChunk) override;

private:
    unsigned char _lastChar;
};

#endif
