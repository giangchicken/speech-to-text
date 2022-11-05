/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QChar>
#include <QList>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextBlockUserData>
#include <QTextEdit>
#include <QTextObject>

namespace src_gui_text_qsyntaxhighlighter {
struct MyHighlighter : public QSyntaxHighlighter
{
    explicit MyHighlighter(QTextDocument *document) : QSyntaxHighlighter(document) { Q_UNUSED(document); }

    void highlightBlock(const QString &text);
    void wrapper();

    QString text;
};

//! [0]
QTextEdit *editor = new QTextEdit;
MyHighlighter *highlighter = new MyHighlighter(editor->document());
//! [0]


//! [1]
void MyHighlighter::highlightBlock(const QString &text)
{
    QTextCharFormat myClassFormat;
    myClassFormat.setFontWeight(QFont::Bold);
    myClassFormat.setForeground(Qt::darkMagenta);

    QRegularExpression expression("\\bMy[A-Za-z]+\\b");
    QRegularExpressionMatchIterator i = expression.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        setFormat(match.capturedStart(), match.capturedLength(), myClassFormat);
    }
}
//! [1]

void MyHighlighter::wrapper() {
//! [2]
QTextCharFormat multiLineCommentFormat;
multiLineCommentFormat.setForeground(Qt::red);

QRegularExpression startExpression("/\\*");
QRegularExpression endExpression("\\*/");

setCurrentBlockState(0);

int startIndex = 0;
if (previousBlockState() != 1)
    startIndex = text.indexOf(startExpression);

while (startIndex >= 0) {
    QRegularExpressionMatch endMatch;
    int endIndex = text.indexOf(endExpression, startIndex, &endMatch);
    int commentLength;
    if (endIndex == -1) {
        setCurrentBlockState(1);
        commentLength = text.length() - startIndex;
    } else {
        commentLength = endIndex - startIndex
                        + endMatch.capturedLength();
    }
    setFormat(startIndex, commentLength, multiLineCommentFormat);
    startIndex = text.indexOf(startExpression,
                              startIndex + commentLength);
}
//! [2]
} // MyHighlighter::wrapper


//! [3]
struct ParenthesisInfo
{
    QChar character;
    int position;
};

struct BlockData : public QTextBlockUserData
{
    QList<ParenthesisInfo> parentheses;
};
//! [3]

} // src_gui_text_qsyntaxhighlighter
