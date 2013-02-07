/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QTest>
#include "longstream_p.h"
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
    LongStream ls;

    ls.cleanupTempFiles();

    QDir dir (ls.tempDir(), "longstream.*");
    QCOMPARE(dir.exists(), true);
    QCOMPARE(dir.entryList().isEmpty(), true);
}
