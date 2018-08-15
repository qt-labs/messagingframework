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

#include "qcharsetdetector.h"
#include "qcharsetdetector_p.h"

#include <unicode/utypes.h>
#include <unicode/uversion.h>
#if (U_ICU_VERSION_MAJOR_NUM > 4) || (U_ICU_VERSION_MAJOR_NUM == 4 && U_ICU_VERSION_MINOR_NUM >=4)
#include <unicode/localpointer.h>
#endif
#include <unicode/uenum.h>
#include <unicode/ucsdet.h>

#include <QString>
#include <QStringList>
#include <QTextCodec>
#include "qmaillog.h"

QCharsetMatchPrivate::QCharsetMatchPrivate()
    : _confidence(0),
      q_ptr(0)
{
}

QCharsetMatchPrivate::QCharsetMatchPrivate(const QCharsetMatchPrivate &other)
    : _name(other._name),
      _language(other._language),
      _confidence(other._confidence),
      q_ptr(0)
{
}

QCharsetMatchPrivate::~QCharsetMatchPrivate()
{
}

QCharsetMatchPrivate &QCharsetMatchPrivate::operator=(const QCharsetMatchPrivate &other)
{
    _name = other._name;
    _language = other._language;
    _confidence = other._confidence;
    return *this;
}

QCharsetMatch::QCharsetMatch()
    : d_ptr(new QCharsetMatchPrivate)
{
    Q_D(QCharsetMatch);
    d->q_ptr = this;
}

QCharsetMatch::QCharsetMatch(const QString name, const QString language, const qint32 confidence)
    : d_ptr(new QCharsetMatchPrivate)
{
    Q_D(QCharsetMatch);
    d->q_ptr = this;
    setName(name);
    setLanguage(language);
    setConfidence(confidence);
}

QCharsetMatch::QCharsetMatch(const QCharsetMatch &other)
    : d_ptr(new QCharsetMatchPrivate(*other.d_ptr))
{
    Q_D(QCharsetMatch);
    d->q_ptr = this;
}

QCharsetMatch::~QCharsetMatch()
{
    delete d_ptr;
}

QCharsetMatch &QCharsetMatch::operator=(const QCharsetMatch &other)
{
    if (this == &other) {
        return *this;
    }
    *d_ptr = *other.d_ptr;
    return *this;
}

bool QCharsetMatch::operator<(const QCharsetMatch &other) const
{
    if(this->confidence() < other.confidence())
        return true;
    else if(this->confidence() == other.confidence()
            && this->language().isEmpty()
            && !other.language().isEmpty())
        return true;
    else
        return false;
}

bool QCharsetMatch::operator>(const QCharsetMatch &other) const
{
    if(this->confidence() > other.confidence())
        return true;
    else if (this->confidence() == other.confidence()
             && !this->language().isEmpty()
             && other.language().isEmpty())
            return true;
    else
        return false;
}

QString QCharsetMatch::name() const
{
    Q_D(const QCharsetMatch);
    return d->_name;
}

void QCharsetMatch::setName(QString name)
{
    Q_D(QCharsetMatch);
    d->_name = name;
}

QString QCharsetMatch::language() const
{
    Q_D(const QCharsetMatch);
    return d->_language;
}

void QCharsetMatch::setLanguage(QString language)
{
    Q_D(QCharsetMatch);
    d->_language = language;
}

qint32 QCharsetMatch::confidence() const
{
    Q_D(const QCharsetMatch);
    return d->_confidence;
}

void QCharsetMatch::setConfidence(qint32 confidence)
{
    Q_D(QCharsetMatch);
    d->_confidence = confidence;
}

QCharsetDetectorPrivate::QCharsetDetectorPrivate()
    : _status(U_ZERO_ERROR),
      _uCharsetDetector(0),
      q_ptr(0)
{
    _uCharsetDetector = ucsdet_open(&_status);
    if(hasError())
        qWarning() << __PRETTY_FUNCTION__ << errorString();
}

QCharsetDetectorPrivate::~QCharsetDetectorPrivate()
{
    ucsdet_close(_uCharsetDetector);
}

bool QCharsetDetectorPrivate::hasError() const
{
    if(U_SUCCESS(_status))
        return false;
    else
        return true;
}

void QCharsetDetectorPrivate::clearError()
{
    _status = U_ZERO_ERROR;
}

QString QCharsetDetectorPrivate::errorString() const
{
    return QString(QLatin1String(u_errorName(_status)));
}

QCharsetDetector::QCharsetDetector()
    : d_ptr(new QCharsetDetectorPrivate)
{
    Q_D(QCharsetDetector);
    d->q_ptr = this;
}

QCharsetDetector::QCharsetDetector(const QByteArray &ba)
    : d_ptr (new QCharsetDetectorPrivate)
{
    Q_D(QCharsetDetector);
    d->q_ptr = this;
    setText(ba);
}

QCharsetDetector::QCharsetDetector(const char *str)
    : d_ptr(new QCharsetDetectorPrivate)
{
    Q_D(QCharsetDetector);
    d->q_ptr = this;
    setText(QByteArray(str));
}

QCharsetDetector::QCharsetDetector(const char *data, int size)
    : d_ptr(new QCharsetDetectorPrivate)
{
    Q_D(QCharsetDetector);
    d->q_ptr = this;
    setText(QByteArray(data, size));
}

QCharsetDetector::~QCharsetDetector()
{
    delete d_ptr;
}

bool QCharsetDetector::hasError() const
{
    Q_D(const QCharsetDetector);
    return d->hasError();
}

void QCharsetDetector::clearError()
{
    Q_D(QCharsetDetector);
    d->clearError();
}

QString QCharsetDetector::errorString() const
{
    Q_D(const QCharsetDetector);
    return d->errorString();
}

void QCharsetDetector::setText(const QByteArray &ba)
{
    Q_D(QCharsetDetector);
    clearError();
    d->_ba = ba;
    d->_baExtended = ba;
    if (!ba.isEmpty()) {
        while (d->_baExtended.size() < 50)
            d->_baExtended += d->_ba;
    } else { // ba is empty, possibly null.
        d->_ba = "";
        d->_baExtended = "";
    }
    // Workaround for libicu bug, it seems to sometimes read past end of input buffer by one byte
    // This was causing messageserver to abnormally terminate when running in valgrind
    d->_baExtended.append(char(0));
    
    ucsdet_setText(d->_uCharsetDetector, d->_baExtended.constData(), int32_t(-1), &(d->_status));
    if(hasError())
        qWarning() << __PRETTY_FUNCTION__ << errorString();
}

QCharsetMatch QCharsetDetector::detect()
{
    // Just call QCharsetDetector::detectAll() and take the first
    // match here instead of using ucsdet_detect() to get only a
    // single match. The list returned by ucsdet_detectAll() maybe
    // tweaked a bit in QCharsetDetector::detectAll() to improve
    // the quality of the detection. Therefore, the first element
    // of the list returned by QCharsetDetector::detectAll() may
    // differ from the single match returned by ucsdet_detect().
    Q_D(QCharsetDetector);
    QList<QCharsetMatch> qCharsetMatchList = detectAll();
    if(hasError()) {
        qWarning() << __PRETTY_FUNCTION__ << errorString();
        return QCharsetMatch();
    }
    if (qCharsetMatchList.isEmpty()) {
        // should never happen, because detectAll() already sets an
        // error if no matches are found which the previous
        // if(hasError()) should detect.
        d->_status = U_CE_NOT_FOUND_ERROR;
        qWarning() << __PRETTY_FUNCTION__
                   << "no matches found at all" << errorString();
        return QCharsetMatch();
    }
    return qCharsetMatchList.first();
}

QList<QCharsetMatch> QCharsetDetector::detectAll()
{
    Q_D(QCharsetDetector);
    clearError();
    // get list of matches from ICU:
    qint32 matchesFound;
    const UCharsetMatch **uCharsetMatch
        = ucsdet_detectAll(d->_uCharsetDetector, &matchesFound, &(d->_status));
    if(hasError()) {
        qWarning() << __PRETTY_FUNCTION__ << errorString();
        return QList<QCharsetMatch>();
    }
    // sometimes the number of matches found by ucsdet_detectAll()
    // maybe 0 (matchesFound == 0) but d->_status has no error. Do not
    // return here with an error if this happens because the fine
    // tuning below may add more matches.  Better check whether no
    // matches were found at all *after* the fine tuning.

    // fill list of matches into a QList<QCharsetMatch>:
    QList<QCharsetMatch> qCharsetMatchList;
    for (qint32 i = 0; i < matchesFound; ++i) {
        QCharsetMatch qCharsetMatch;
        qCharsetMatch.setName(
            QString::fromLatin1(ucsdet_getName(uCharsetMatch[i], &(d->_status))));
        if(hasError()) {
            qWarning() << __PRETTY_FUNCTION__ << errorString();
            return QList<QCharsetMatch>();
        }
        qCharsetMatch.setConfidence(
            static_cast<qint32>(ucsdet_getConfidence (uCharsetMatch[i], &(d->_status))));
        if(hasError()) {
            qWarning() << __PRETTY_FUNCTION__ << errorString();
            return QList<QCharsetMatch>();
        }
        qCharsetMatch.setLanguage(
            QString::fromLatin1(ucsdet_getLanguage(uCharsetMatch[i], &(d->_status))));
        if(hasError()) {
            qWarning() << __PRETTY_FUNCTION__ << errorString();
            return QList<QCharsetMatch>();
        }
        qCharsetMatchList << qCharsetMatch;
    }
    if(d->_allDetectableCharsets.isEmpty())
        getAllDetectableCharsets();
    // libicu sometimes does not detect single byte encodings at all
    // even if they can encode the input without error. This seems to
    // contradict the documentation on
    // http://icu-project.org/apiref/icu4c/ucsdet_8h.html which says:
    //
    //     A confidence value of ten does have a general meaning - it is
    //     used for charsets that can represent the input data, but for
    //     which there is no other indication that suggests that the
    //     charset is the correct one. Pure 7 bit ASCII data, for example,
    //     is compatible with a great many charsets, most of which will
    //     appear as possible matches with a confidence of 10.
    //
    // But if such a single byte encoding has been set as the declared
    // encoding, it should at least be tried, therefore add it here to
    // the list of matches with the confidence value of 10. If it
    // cannot encode the complete input, the iteration over the list
    // of matches will detect that and remove it again.
    if(!d->_declaredEncoding.isEmpty()
        && (d->_declaredEncoding.startsWith(QLatin1String("ISO-8859-"))
            || d->_declaredEncoding.startsWith(QLatin1String("windows-12"))
            || d->_declaredEncoding.startsWith(QLatin1String("KOI8"))))
            qCharsetMatchList << QCharsetMatch(d->_declaredEncoding, QString(), 10);
    // Similar as for declaredEncoding, when declaredLocale is used
    // and it is a locale where the legacy encoding is a single byte
    // encoding, it should at least be tried, therefore add the legacy
    // single byte encoding for the declared locale here.  If it
    // cannot encode the complete input, it will be removed again
    // later.  Multibyte encodings like Shift_JIS, EUC-JP, Big5,
    // etc. ...  do not need to be added, contrary to the single byte
    // encodings I could find no case where the matches returned by
    // libicu did omit a multibyte encoding when it should have been
    // included.
    if(!d->_declaredLocale.isEmpty()) {
        QString language = d->_declaredLocale.left(2);
        if(language ==  QLatin1String("ru")) {
            qCharsetMatchList << QCharsetMatch(QLatin1String("KOI8-R"), language, 10);
            qCharsetMatchList << QCharsetMatch(QLatin1String("windows-1251"), language, 10);
            qCharsetMatchList << QCharsetMatch(QLatin1String("ISO-8859-5"), language, 10);
        }
        else if(language == QLatin1String("tr"))
            qCharsetMatchList << QCharsetMatch(QLatin1String("ISO-8859-9"), language, 10);
        else if(language == QLatin1String("el"))
            qCharsetMatchList << QCharsetMatch(QLatin1String("ISO-8859-7"), language, 10);
        else if(language == QLatin1String("en")
                || language == QLatin1String("da")
                || language == QLatin1String("de")
                || language == QLatin1String("es")
                || language == QLatin1String("fi")
                || language == QLatin1String("fr")
                || language == QLatin1String("it")
                || language == QLatin1String("nl")
                || language == QLatin1String("no")
                || language == QLatin1String("nn")
                || language == QLatin1String("nb")
                || language == QLatin1String("pt")
                || language == QLatin1String("sv"))
            qCharsetMatchList << QCharsetMatch(QLatin1String("ISO-8859-1"), language, 10);
        else if(language == QLatin1String("cs")
                || language == QLatin1String("hu")
                || language == QLatin1String("pl")
                || language == QLatin1String("ro"))
            qCharsetMatchList << QCharsetMatch(QLatin1String("ISO-8859-1"), language, 10);
        else if(language == QLatin1String("ar")
                || language == QLatin1String("fa")
                || language == QLatin1String("ur"))
            qCharsetMatchList << QCharsetMatch(QLatin1String("ISO-8859-6"), language, 10);
        else if(language == QLatin1String("he"))
            qCharsetMatchList << QCharsetMatch(QLatin1String("ISO-8859-8"), language, 10);
    }
    // iterate over the detected matches and do some fine tuning:
    bool sortNeeded = false;
    qint32 koi8rConfidence = 0;
    qint32 iso88595Confidence = 0;
    qint32 windows1251Confidence = 0;
    QList<QCharsetMatch>::iterator it = qCharsetMatchList.begin();
    while(it != qCharsetMatchList.end()) {
        if((*it).name() == QLatin1String("KOI8-R"))
            koi8rConfidence += (*it).confidence();
        if((*it).name() == QLatin1String("ISO-8859-5"))
            iso88595Confidence += (*it).confidence();
        if((*it).name() == QLatin1String("windows-1251"))
            windows1251Confidence += (*it).confidence();
        if((*it).name() == QLatin1String("ISO-2022-JP")) {
            // non-Japanese text in ISO-2022-JP encoding is possible
            // but very unlikely:
            (*it).setLanguage(QLatin1String("ja"));
        }
        if((*it).name() == QLatin1String("UTF-8")
           && (*it).confidence() >= 80 && (*it).confidence() < 99) {
            // Actually libicu currently only returns confidence
            // values of 100, 80, 25, and 10 for UTF-8.  A value of 80
            // can mean two things:
            //
            // 1)  (hasBOM && numValid > numInvalid*10)
            // 2)  (numValid > 0 && numInvalid == 0)
            //
            // If it is case 1), the match will be removed anyway by
            // the check below which tests whether the complete input
            // can be encoded. I.e. we don’t need to care about this.
            //
            // If it is case 2) *and* the check below whether the
            // complete input can be encoded does not remove it, we
            // have valid UTF-8 and it is very unlikely that it is
            // anything else, therefore I think the confidence of 80
            // is too low and should be increased.
            // With a confidence of only 80, a longer ASCII text with
            // less than 4 UTF-8 characters will detect as ISO-8859-1
            // which is most certainly wrong.
            (*it).setConfidence(99);
            sortNeeded = true;
        }
        if(!d->_declaredEncoding.isEmpty()
           && (*it).name() == d->_declaredEncoding
           && (*it).confidence() == 10) {
            // A confidence value of 10 means the charset can
            // represent the input data, but there is no other
            // indication that suggests that the charset is the
            // correct one. But if the user has set this to be the
            // declared encoding, it should be preferred over the
            // other encodings which also got confidence 10 (there are
            // often many with confidence 10). Do not increase the
            // confidence too much though in order not to override
            // real evidence that the input does really use something
            // different than the declared encoding.
            (*it).setConfidence(40);
            sortNeeded = true;
        }
        if(!d->_declaredLocale.isEmpty()
           && d->_declaredLocale.startsWith((*it).language())
           && (*it).confidence() == 10) {
            // A confidence value of 10 means the charset can
            // represent the input data, but there is no other
            // indication that suggests that the charset is the
            // correct one. But if the detected language for this
            // charset matches the language declared by the user, this
            // charset should be preferred over the others which also
            // got confidence 10 (there are often many with confidence
            // 10). Do not increase the confidence too much though in
            // order not to override real evidence that the input does
            // really use something different than the declared
            // encoding.  Use a slightly lower value than for the
            // declared encoding. Setting the declared encoding
            // is more precise and should have somewhat higher priority
            if (d->_declaredLocale.startsWith(QLatin1String("ru"))) {
                // Treat the Russian setDeclaredLocale("ru") case a
                // bit different than the single byte encodings for
                // other languages: Only increase the weight of
                // Russian encodings if setDeclaredLocale("ru") has
                // been used if libicu has really detected the same
                // Russian encoding as well. libicu usually detects
                // these Russian encodings with very low confidences <
                // 10 for short input.  But if we are already pretty
                // sure that it is Russian because of
                // setDeclaredLocale("ru"), then these low confidences
                // detected by libicu seem to be useful to distinguish
                // between the different Russian legacy encodings.
                //
                // If the setDeclareLocale("ru") has been used, the
                // accumulated confidence for the Russian single byte
                // encoding is 10 (because of setDeclaredLocale("ru"))
                // plus whatever libicu has detected. If libicu has
                // not detected anything, the accumulated confidence
                // is exactly 10 here and there is no way to
                // distinguish between the Russian legacy
                // encodings. Therefore, don’t increase the confidence
                // if the accumulated confidence is not > 10.
                //
                // But if libicu has detected something with small
                // confidence, the accumulated confidence is 10 plus
                // something small. In that case, adding something
                // around 20 seems to work reasonably well.
                //
                // I add 20 to the confidence for KOI8-R and
                // ISO-8859-5 but 21 to the confidence for
                // windows-1251 to prefer windows-1251 a little bit
                // over ISO-8859-5.
                if((*it).name() == QLatin1String("KOI8-R")
                   && koi8rConfidence > 10 && koi8rConfidence < 30)
                    (*it).setConfidence(20 + koi8rConfidence);
                else if((*it).name() == QLatin1String("ISO-8859-5")
                   && iso88595Confidence > 10 && iso88595Confidence < 30)
                    (*it).setConfidence(20 + iso88595Confidence);
                else if((*it).name() == QLatin1String("windows-1251")
                   && windows1251Confidence > 10 && windows1251Confidence < 30)
                    (*it).setConfidence(21 + windows1251Confidence);
            }
            else if ((d->_declaredLocale.contains(QLatin1String("TW"))
                || d->_declaredLocale.contains(QLatin1String("HK"))
                || d->_declaredLocale.contains(QLatin1String("MO")))
               && (*it).name() == QLatin1String("Big5")) {
                 // Traditional Chinese, Big5 more likely
                (*it).setConfidence(39);
            }
            else if ((d->_declaredLocale.contains(QLatin1String("CN"))
                     || d->_declaredLocale.contains(QLatin1String("SG"))
                     || d->_declaredLocale == QLatin1String("zh"))
                    && (*it).name() == QLatin1String("GB18030")) {
                // Simplified Chinese, GB18030/GB2312 more likely.
                // Simplified Chinese is also assumed if only “zh”
                // is set. If the variant is unknown, simplified
                // Chinese seems a bit more likely. On top of that,
                // the settings application sets only “zh” for
                // simplified Chinese and the translations for
                // simplified Chinese are also in files like
                // “foo_zh.qm” which makes simplified Chinese more
                // likely when only “zh” is set on the device (see
                // also NB#242154).
                (*it).setConfidence(39);
            }
            else {
                (*it).setConfidence(38);
            }
            sortNeeded = true;
        }
        if(!d->_allDetectableCharsets.contains((*it).name())) {
            // remove matches for charsets not supported by QTextCodec
            // then it is probably some weird charset we cannot use anyway
            it = qCharsetMatchList.erase(it);
        }
        else {
            // test whether the complete input text can be encoded
            // using this match, if not remove the match
            clearError();
            text(*it);
            if(hasError()) {
                // qMailLog(Messaging) << __PRETTY_FUNCTION__
                //          << "removing match" << (*it).name()
                //          << "because it cannot encode the complete input"
                //          << errorString();
                it = qCharsetMatchList.erase(it);
                clearError();
            }
            else
                ++it;
        }
    }
    // sort the list of matches again if confidences have been changed:
    if(sortNeeded)
        std::sort(qCharsetMatchList.begin(), qCharsetMatchList.end(),
              std::greater<QCharsetMatch>());
    if(qCharsetMatchList.isEmpty()) {
        // is there any better status to describe this case?
        d->_status = U_CE_NOT_FOUND_ERROR;
        qWarning() << __PRETTY_FUNCTION__
                 << "number of matches found=0"
                 << errorString();
        return QList<QCharsetMatch>();
    }
    return qCharsetMatchList;
}

QString QCharsetDetector::text(const QCharsetMatch &charsetMatch)
{
    Q_D(QCharsetDetector);
    clearError();
    QTextCodec *codec
        = QTextCodec::codecForName(charsetMatch.name().toLatin1());
    if (codec == NULL) { // there is no codec matching the name
        d->_status = U_ILLEGAL_ARGUMENT_ERROR;
        qWarning() << __PRETTY_FUNCTION__
                 << "no codec for the name" << charsetMatch.name()
                 << errorString();
        // return empty string to indicate that no conversion is possible:
        return QString();
    }
    else {
        QTextCodec::ConverterState state;
        QString text =
            codec->toUnicode(d->_ba.constData(), d->_ba.size(), &state);
        if (state.invalidChars > 0)
            d->_status = U_INVALID_CHAR_FOUND;
        return text;
    }
}

void QCharsetDetector::setDeclaredLocale(const QString &locale)
{
    Q_D(QCharsetDetector);
    clearError();
    d->_declaredLocale = locale;
}

void QCharsetDetector::setDeclaredEncoding(const QString &encoding)
{
    Q_D(QCharsetDetector);
    clearError();
    d->_declaredEncoding = encoding;
    if (d->_declaredEncoding == QLatin1String("GB2312"))
        d->_declaredEncoding = QLatin1String("GB18030");
    ucsdet_setDeclaredEncoding(d->_uCharsetDetector,
                               d->_declaredEncoding.toLatin1().constData(),
                               int32_t(-1),
                               &(d->_status));
    if(hasError())
        qWarning() << __PRETTY_FUNCTION__ << errorString();
}

QStringList QCharsetDetector::getAllDetectableCharsets()
{
    Q_D(QCharsetDetector);

    if (!d->_allDetectableCharsets.isEmpty())
        return d->_allDetectableCharsets;

    // Codecs supported by QTextCodec (Qt 4.7):
    //
    // ISO-2022-JP JIS7 EUC-KR GB2312 Big5 Big5-ETen CP950 GB18030
    // EUC-JP Shift_JIS SJIS MS_Kanji System UTF-8 ISO-8859-1 latin1
    // CP819 IBM819 iso-ir-100 csISOLatin1 ISO-8859-15 latin9 UTF-32LE
    // UTF-32BE UTF-32 UTF-16LE UTF-16BE UTF-16 mulelao-1 roman8
    // hp-roman8 csHPRoman8 TIS-620 ISO 8859-11 WINSAMI2 WS2 Apple
    // Roman macintosh MacRoman windows-1258 CP1258 windows-1257
    // CP1257 windows-1256 CP1256 windows-1255 CP1255 windows-1254
    // CP1254 windows-1253 CP1253 windows-1252 CP1252 windows-1251
    // CP1251 windows-1250 CP1250 IBM866 CP866 csIBM866 IBM874 CP874
    // IBM850 CP850 csPC850Multilingual ISO-8859-16 iso-ir-226 latin10
    // ISO-8859-14 iso-ir-199 latin8 iso-celtic ISO-8859-13
    // ISO-8859-10 iso-ir-157 latin6 ISO-8859-10:1992 csISOLatin6
    // ISO-8859-9 iso-ir-148 latin5 csISOLatin5 ISO-8859-8 ISO
    // 8859-8-I iso-ir-138 hebrew csISOLatinHebrew ISO-8859-7 ECMA-118
    // greek iso-ir-126 csISOLatinGreek ISO-8859-6 ISO-8859-6-I
    // ECMA-114 ASMO-708 arabic iso-ir-127 csISOLatinArabic ISO-8859-5
    // cyrillic iso-ir-144 csISOLatinCyrillic ISO-8859-4 latin4
    // iso-ir-110 csISOLatin4 ISO-8859-3 latin3 iso-ir-109 csISOLatin3
    // ISO-8859-2 latin2 iso-ir-101 csISOLatin2 KOI8-U KOI8-RU KOI8-R
    // csKOI8R Iscii-Mlm Iscii-Knd Iscii-Tlg Iscii-Tml Iscii-Ori
    // Iscii-Gjr Iscii-Pnj Iscii-Bng Iscii-Dev TSCII GBK gb2312.1980-0
    // gbk-0 CP936 MS936 windows-936 jisx0201*-0 jisx0208*-0
    // ksc5601.1987-0 cp949 Big5-HKSCS big5-0 big5hkscs-0

    QStringList availableCodecsQt;
    foreach(const QByteArray &ba, QTextCodec::availableCodecs())
        availableCodecsQt << QString::fromLatin1(ba);

    // Charsets detectable by libicu 4.4.2:
    QStringList allDetectableCharsetsICU;
    allDetectableCharsetsICU
    << QLatin1String("UTF-8")
    << QLatin1String("UTF-16BE")
    << QLatin1String("UTF-16LE")
    << QLatin1String("UTF-32BE")
    << QLatin1String("UTF-32LE")
    << QLatin1String("ISO-8859-1")
    << QLatin1String("ISO-8859-2")
    << QLatin1String("ISO-8859-5")
    << QLatin1String("ISO-8859-6")
    << QLatin1String("ISO-8859-7")
    << QLatin1String("ISO-8859-8-I")
    << QLatin1String("ISO-8859-8")
    << QLatin1String("ISO-8859-9")
    << QLatin1String("KOI8-R")
    << QLatin1String("Shift_JIS")
    << QLatin1String("GB18030")
    << QLatin1String("EUC-JP")
    << QLatin1String("EUC-KR")
    << QLatin1String("Big5")
    << QLatin1String("ISO-2022-JP")
    << QLatin1String("ISO-2022-KR")
    << QLatin1String("ISO-2022-CN")
    << QLatin1String("IBM424_rtl")
    << QLatin1String("IBM424_ltr")
    << QLatin1String("IBM420_rtl")
    << QLatin1String("IBM420_ltr")
    << QLatin1String("windows-1250")
    << QLatin1String("windows-1251")
    << QLatin1String("windows-1252")
    << QLatin1String("windows-1253")
    << QLatin1String("windows-1255")
    << QLatin1String("windows-1256")
    << QLatin1String("windows-1254");

    // The charsets detectable by libicu can be determined by
    // ucsdet_getAllDetectableCharsets() and the documentation for
    // that function at
    // http://icu-project.org/apiref/icu4c/ucsdet_8h.html says:
    //
    //     “The state of the Charset detector that is passed in does
    //     not affect the result of this function, but requiring a
    //     valid, open charset detector as a parameter insures that
    //     the charset detection service has been safely initialized
    //     and that the required detection data is available.”
    //
    // but that does not seem to be completely true, in fact it
    // *does* depend on the state of the charset detector. For example
    // sometimes "windows-1250" *is* among the returned charsets.
    // This happens if some non-ASCII text
    // is in the detector and a detection is attempted and *then*
    // ucsdet_getAllDetectableCharsets() is called.
    // And sometimes "windows-1250" is *not* among the returned
    // charsets. This happens when an empty charset detector is created
    // and then ucsdet_getAllDetectableCharsets() is called.
    // If ucsdet_getAllDetectableCharsets() has been called once
    // the list of returned charsets never seems to change anymore,
    // even if the text in the detector is changed again and
    // another detection attempted which would result in a different
    // list if ucsdet_getAllDetectableCharsets() were called first
    // in that state.
    //
    // Sometimes ucsdet_getAllDetectableCharsets() reports charsets
    // multiple times depending on the number of languages it can
    // detect for that charsets, i.e. it may report ISO-8859-2 four
    // times because it can detect the languages “cs”, “hu”,
    // “pl”, and “ro” with that charset.
    //
    // This looks like a bug to me, to get a reliable list,
    // I have hardcoded the complete list of charsets which
    // ucsdet_getAllDetectableCharsets() can possibly return
    // for all states of the detector above.
    //
    // Therefore, the following code should not any extra charsets
    // anymore, at least not for libicu 4.4.2:
    clearError();
    UEnumeration *en =
        ucsdet_getAllDetectableCharsets(d->_uCharsetDetector, &(d->_status));
    if (!hasError()) {
        qint32 len;
        const UChar *uc;
        while ((uc = uenum_unext(en, &len, &(d->_status))) != NULL) {
            if(uc && !hasError())
                allDetectableCharsetsICU << QString::fromUtf16(uc, len);
        }
    }
    uenum_close(en);

    // remove all charsets not supported by QTextCodec and all duplicates:
    foreach(const QString &cs, allDetectableCharsetsICU) {
        if(availableCodecsQt.contains(cs) && !d->_allDetectableCharsets.contains(cs))
            d->_allDetectableCharsets << cs;
    }

    std::sort(d->_allDetectableCharsets.begin(), d->_allDetectableCharsets.end());

    return d->_allDetectableCharsets;
}

void QCharsetDetector::enableInputFilter(bool enable)
{
    Q_D(QCharsetDetector);
    clearError();
    ucsdet_enableInputFilter(d->_uCharsetDetector, UBool(enable));
}

bool QCharsetDetector::isInputFilterEnabled()
{
    Q_D(QCharsetDetector);
    clearError();
    return bool(ucsdet_isInputFilterEnabled(d->_uCharsetDetector));
}
