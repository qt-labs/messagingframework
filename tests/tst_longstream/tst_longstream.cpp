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

#include <QObject>
#include <QTest>
#include <private/longstream_p.h>
#include <ctype.h>
#include <QDir>

/*
    This class primarily tests that LongStream class correctly stores messages.
*/
class tst_LongStream : public QObject
{
    Q_OBJECT

public:
    tst_LongStream() {}
    virtual ~tst_LongStream() {}

private slots:

    void test_new_stream();
    void test_status();
    void test_errorMessage();
    void test_temp_files();
};

QTEST_MAIN(tst_LongStream)

#include "tst_longstream.moc"

void tst_LongStream::test_new_stream()
{
    // constructor
    LongStream ls;
    QCOMPARE(ls.status(), LongStream::Ok);

    QString filename = ls.fileName();
    QVERIFY(filename.indexOf("longstream") != -1);
    QVERIFY(filename.indexOf(QRegExp("longstream\\.\\S{6,6}$")) != -1);

    // new data
    QString data("This is a new message to be stored in LongStream for testing purpose."
                 "\'The quick brown fox jumps over the lazy dog\' is a sentence containing all the english alphabets and also adds some meaning to the sentence."
                 ""
                 "Allow the president to invade a neighboring nation, whenever he shall deem it necessary to repel an invasion, and you allow him to do so whenever he may choose to say he deems it necessary for such a purpose - and you allow him to make war at pleasure."
                 "--Abraham Lincoln.");
    ls.append(data);
    QCOMPARE(ls.length(), data.length());

    // reset
    ls.reset();

    QCOMPARE(filename, ls.fileName());
    QCOMPARE(ls.length(), 0);
    QCOMPARE(ls.readAll(), QString());
    QCOMPARE(ls.status(), LongStream::Ok);

    // more data
    ls.append(data);
    QCOMPARE(ls.length(), data.length());

    ls.append(data);
    QCOMPARE(ls.length(), data.length() * 2);

    ls.append(data);
    QCOMPARE(ls.length(), data.length() * 3);

    // detach
    QString old_filename = ls.detach();

    QVERIFY(old_filename != ls.fileName());
    QCOMPARE(ls.length(), 0);
    QCOMPARE(ls.readAll(), QString());
    QCOMPARE(ls.status(), LongStream::Ok);

}

void tst_LongStream::test_status()
{
    LongStream ls;

    ls.setStatus(LongStream::Ok);
    QCOMPARE(ls.status(), LongStream::Ok);

    ls.setStatus(LongStream::OutOfSpace);
    QCOMPARE(ls.status(), LongStream::OutOfSpace);

    ls.resetStatus();
    QCOMPARE(ls.status(), LongStream::Ok);

    ls.updateStatus();
    QCOMPARE(ls.status(), LongStream::Ok);
}

void tst_LongStream::test_errorMessage()
{
    LongStream ls;

    QString err = ls.errorMessage();
    QCOMPARE(err.isEmpty(), false);

    QString prefix("error prefix: ");
    QCOMPARE(ls.errorMessage(prefix), prefix+err);
}

void tst_LongStream::test_temp_files()
{
    // Create a LongStream. We do it in a block since it holds a file handle. We
    // need this file handle to be closed so that we can get rid of it on
    // Windows (as Windows doesn't allow the removal of closed files).
    {
        LongStream ls;
    }

    LongStream::cleanupTempFiles();

    QDir dir (LongStream::tempDir(), "longstream.*");
    QCOMPARE(dir.exists(), true);
    QVERIFY2(dir.entryList().isEmpty(), qPrintable(dir.entryList().join(" ")));
}
