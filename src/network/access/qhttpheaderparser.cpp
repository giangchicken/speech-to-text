/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhttpheaderparser_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

// both constants are taken from the default settings of Apache
// see: http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfieldsize and
// http://httpd.apache.org/docs/2.2/mod/core.html#limitrequestfields
static const int MAX_HEADER_FIELD_SIZE = 8 * 1024;
static const int MAX_HEADER_FIELDS = 100;

QHttpHeaderParser::QHttpHeaderParser()
    : statusCode(100) // Required by tst_QHttpNetworkConnection::ignoresslerror(failure)
    , majorVersion(0)
    , minorVersion(0)
{
}

void QHttpHeaderParser::clear()
{
    statusCode = 100;
    majorVersion = 0;
    minorVersion = 0;
    reasonPhrase.clear();
    fields.clear();
}

static bool fieldNameCheck(QByteArrayView name)
{
    static constexpr QByteArrayView otherCharacters("!#$%&'*+-.^_`|~");
    static const auto fieldNameChar = [](char c) {
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')
                || otherCharacters.contains(c);
    };

    return name.size() > 0 && std::all_of(name.begin(), name.end(), fieldNameChar);
}

bool QHttpHeaderParser::parseHeaders(QByteArrayView header)
{
    // see rfc2616, sec 4 for information about HTTP/1.1 headers.
    // allows relaxed parsing here, accepts both CRLF & LF line endings
    Q_ASSERT(fields.isEmpty());
    const auto hSpaceStart = [](QByteArrayView h) {
        return h.startsWith(' ') || h.startsWith('\t');
    };
    // Headers, if non-empty, start with a non-space and end with a newline:
    if (hSpaceStart(header) || (header.size() && !header.endsWith('\n')))
        return false;

    while (int tail = header.endsWith("\n\r\n") ? 2 : header.endsWith("\n\n") ? 1 : 0)
        header.chop(tail);

    QList<QPair<QByteArray, QByteArray>> result;
    while (header.size()) {
        const int colon = header.indexOf(':');
        if (colon == -1) // if no colon check if empty headers
            return result.size() == 0 && (header == "\n" || header == "\r\n");
        if (result.size() >= MAX_HEADER_FIELDS)
            return false;
        QByteArrayView name = header.first(colon);
        if (!fieldNameCheck(name))
            return false;
        header = header.sliced(colon + 1);
        QByteArray value;
        int valueSpace = MAX_HEADER_FIELD_SIZE - name.size() - 1;
        do {
            const int endLine = header.indexOf('\n');
            Q_ASSERT(endLine != -1);
            auto line = header.first(endLine); // includes space
            valueSpace -= line.size() - (line.endsWith('\r') ? 1 : 0);
            if (valueSpace < 0)
                return false;
            line = line.trimmed();
            if (line.size()) {
                if (value.size())
                    value += ' ' + line.toByteArray();
                else
                    value = line.toByteArray();
            }
            header = header.sliced(endLine + 1);
        } while (hSpaceStart(header));
        Q_ASSERT(name.size() + 1 + value.size() <= MAX_HEADER_FIELD_SIZE);
        result.append(qMakePair(name.toByteArray(), value));
    }

    fields = result;
    return true;
}

bool QHttpHeaderParser::parseStatus(QByteArrayView status)
{
    // from RFC 2616:
    //        Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    //        HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
    // that makes: 'HTTP/n.n xxx Message'
    // byte count:  0123456789012

    static const int minLength = 11;
    static const int dotPos = 6;
    static const int spacePos = 8;
    static const char httpMagic[] = "HTTP/";

    if (status.length() < minLength
        || !status.startsWith(httpMagic)
        || status.at(dotPos) != '.'
        || status.at(spacePos) != ' ') {
        // I don't know how to parse this status line
        return false;
    }

    // optimize for the valid case: defer checking until the end
    majorVersion = status.at(dotPos - 1) - '0';
    minorVersion = status.at(dotPos + 1) - '0';

    int i = spacePos;
    int j = status.indexOf(' ', i + 1);
    const QByteArrayView code = j > i ? status.sliced(i + 1, j - i - 1)
                                      : status.sliced(i + 1);

    bool ok;
    statusCode = code.toInt(&ok);

    reasonPhrase = j > i ? QString::fromLatin1(status.sliced(j + 1))
                         : QString();

    return ok && uint(majorVersion) <= 9 && uint(minorVersion) <= 9;
}

const QList<QPair<QByteArray, QByteArray> >& QHttpHeaderParser::headers() const
{
    return fields;
}

QByteArray QHttpHeaderParser::firstHeaderField(const QByteArray &name,
                                               const QByteArray &defaultValue) const
{
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        if (name.compare(it->first, Qt::CaseInsensitive) == 0)
            return it->second;
    }
    return defaultValue;
}

QByteArray QHttpHeaderParser::combinedHeaderValue(const QByteArray &name, const QByteArray &defaultValue) const
{
    const QList<QByteArray> allValues = headerFieldValues(name);
    if (allValues.isEmpty())
        return defaultValue;
    else
        return allValues.join(", ");
}

QList<QByteArray> QHttpHeaderParser::headerFieldValues(const QByteArray &name) const
{
    QList<QByteArray> result;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it)
        if (name.compare(it->first, Qt::CaseInsensitive) == 0)
            result += it->second;

    return result;
}

void QHttpHeaderParser::removeHeaderField(const QByteArray &name)
{
    auto firstEqualsName = [&name](const QPair<QByteArray, QByteArray> &header) {
        return name.compare(header.first, Qt::CaseInsensitive) == 0;
    };
    fields.removeIf(firstEqualsName);
}

void QHttpHeaderParser::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    removeHeaderField(name);
    fields.append(qMakePair(name, data));
}

void QHttpHeaderParser::prependHeaderField(const QByteArray &name, const QByteArray &data)
{
    fields.prepend(qMakePair(name, data));
}

void QHttpHeaderParser::appendHeaderField(const QByteArray &name, const QByteArray &data)
{
    fields.append(qMakePair(name, data));
}

void QHttpHeaderParser::clearHeaders()
{
    fields.clear();
}

int QHttpHeaderParser::getStatusCode() const
{
    return statusCode;
}

void QHttpHeaderParser::setStatusCode(int code)
{
    statusCode = code;
}

int QHttpHeaderParser::getMajorVersion() const
{
    return majorVersion;
}

void QHttpHeaderParser::setMajorVersion(int version)
{
    majorVersion = version;
}

int QHttpHeaderParser::getMinorVersion() const
{
    return minorVersion;
}

void QHttpHeaderParser::setMinorVersion(int version)
{
    minorVersion = version;
}

QString QHttpHeaderParser::getReasonPhrase() const
{
    return reasonPhrase;
}

void QHttpHeaderParser::setReasonPhrase(const QString &reason)
{
    reasonPhrase = reason;
}

QT_END_NAMESPACE
