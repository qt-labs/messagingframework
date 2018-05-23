#include "imapfoldernamecoding.h"

#include <qstring.h>
#include <qlist.h>

QString encodeModifiedBase64(QString in)
{
    // Modified Base64 chars pattern
    const QString encodingSchema = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,";

    QString result;

    QList<ushort> buf;
    unsigned short tmp;
    int i;

    // chars to numeric
    for(i = 0; i < in.length(); i++){
        buf.push_back(in[i].unicode());
    }

    // adding &
    result.push_back('&');

    i = 0;

    // encode every 6 bits separately in pattern by 3 symbols
    while(i < buf.length()){

        result.push_back(encodingSchema[(buf[i] & 0xfc00) >> 10]);

        result.push_back(encodingSchema[(buf[i] & 0x3f0) >> 4]);

        tmp = 0;
        tmp |= ((buf[i] & 0xf) << 12);
        if (i + 1 < buf.length())
        {
            i++;
            tmp |= ((buf[i] & 0xc000) >> 4);
            result.push_back(encodingSchema[tmp >> 10]);
        } else {
            result.push_back(encodingSchema[tmp >> 10]);
            break;
        }

        result.push_back(encodingSchema[(buf[i] & 0x3f00) >> 8]);

        result.push_back(encodingSchema[(buf[i] & 0xfc) >> 2]);

        tmp = 0;
        tmp |= ((buf[i] & 0x3) << 14);
        if (i + 1 < buf.length())
        {
            i++;
            tmp |= ((buf[i] & 0xf000) >> 2);
            result.push_back(encodingSchema[tmp >> 10]);
        } else {
            result.push_back(encodingSchema[tmp >> 10]);
            break;
        }

        result.push_back(encodingSchema[(buf[i] & 0xfc0) >> 6]);

        result.push_back(encodingSchema[buf[i] & 0x3f]);

        i++;
    }

    // adding -
    result.push_back('-');

    return result;
}

QString encodeModUTF7(QString in)
{
    int startIndex = 0;
    int endIndex = 0;

    while (startIndex < in.length())
    {
        // insert '_' after '&'
        if (in[startIndex] == '&')
        {
            startIndex++;
            in.insert(startIndex, '-');
            continue;
        }

        if (in[startIndex].unicode() < 0x20 || in[startIndex].unicode() > 0x7e)
        {
            // get non-US-ASCII part
            endIndex = startIndex;
            while(endIndex < in.length() && (in[endIndex].unicode() < 0x20 || in[endIndex].unicode() > 0x7e))
                endIndex++;

            // encode non-US-ASCII part
            QString unicodeString = in.mid(startIndex,(endIndex - startIndex));
            QString mbase64 = encodeModifiedBase64(unicodeString);

            // insert the encoded string
            in.remove(startIndex,(endIndex-startIndex));
            in.insert(startIndex, mbase64);

            // set start index to the end of the encoded part
            startIndex += mbase64.length() - 1;
        }
        startIndex++;
    }
    return in;
}

QString encodeFolderName(const QString &name)
{
        return encodeModUTF7(name);
}

QString decodeModifiedBase64(QString in)
{
    //remove  & -
    in.remove(0,1);
    in.remove(in.length()-1,1);

    if(in.isEmpty())
        return "&";

    QByteArray buf(in.length(),static_cast<char>(0));
    QByteArray out(in.length() * 3 / 4 + 2,static_cast<char>(0));

    //chars to numeric
    QByteArray latinChars = in.toLatin1();
    for (int x = 0; x < in.length(); x++) {
        int c = latinChars[x];
        if ( c >= 'A' && c <= 'Z')
            buf[x] = c - 'A';
        if ( c >= 'a' && c <= 'z')
            buf[x] = c - 'a' + 26;
        if ( c >= '0' && c <= '9')
            buf[x] = c - '0' + 52;
        if ( c == '+')
            buf[x] = 62;
        if ( c == ',')
            buf[x] = 63;
    }

    int i = 0; //in buffer index
    int j = i; //out buffer index

    unsigned char z;
    QString result;

    while(i+1 < buf.size())
    {
        out[j] = buf[i] & (0x3F); //mask out top 2 bits
        out[j] = out[j] << 2;
        z = buf[i+1] >> 4;
        out[j] = (out[j] | z);      //first byte retrieved

        i++;
        j++;

        if(i+1 >= buf.size())
            break;

        out[j] = buf[i] & (0x0F);   //mask out top 4 bits
        out[j] = out[j] << 4;
        z = buf[i+1] >> 2;
        z &= 0x0F;
        out[j] = (out[j] | z);      //second byte retrieved

        i++;
        j++;

        if(i+1 >= buf.size())
            break;

        out[j] = buf[i] & 0x03;   //mask out top 6 bits
        out[j] = out[j] <<  6;
        z = buf[i+1];
        out[j] = out[j] | z;  //third byte retrieved

        i+=2; //next byte
        j++;
    }

    //go through the buffer and extract 16 bit unicode network byte order
    for(int z = 0; z < out.count(); z+=2) {
        unsigned short outcode = 0x0000;
        outcode = out[z];
        outcode <<= 8;
        outcode &= 0xFF00;

        unsigned short b = 0x0000;
        b = out[z+1];
        b &= 0x00FF;
        outcode = outcode | b;
        if(outcode)
            result += QChar(outcode);
    }

    return result;
}

QString decodeModUTF7(QString in)
{
    QRegExp reg("&[^&-]*-");

    int startIndex = 0;
    int endIndex = 0;

    startIndex = in.indexOf(reg,endIndex);
    while (startIndex != -1) {
        endIndex = startIndex;
        while(endIndex < in.length() && in[endIndex] != '-')
            endIndex++;
        endIndex++;

        //extract the base64 string from the input string
        QString mbase64 = in.mid(startIndex,(endIndex - startIndex));
        QString unicodeString = decodeModifiedBase64(mbase64);

        //remove encoding
        in.remove(startIndex,(endIndex-startIndex));
        in.insert(startIndex,unicodeString);

        endIndex = startIndex + unicodeString.length();
        startIndex = in.indexOf(reg,endIndex);
    }

    return in;
}

QString decodeFolderName(const QString &name)
{
    return decodeModUTF7(name);
}
