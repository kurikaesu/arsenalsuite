
#ifndef HTML_HIGHLIGHTER_H
#define HTML_HIGHLIGHTER_H

#include <qsyntaxhighlighter.h>

#include "stonegui.h"

class STONEGUI_EXPORT HtmlHighlighter : public QSyntaxHighlighter
{
Q_OBJECT

public:
	enum Construct {
		Entity,
		Tag,
		Comment,
		Attribute,
		Value,
		LastConstruct = Value
	};

	HtmlHighlighter(QTextEdit *textEdit);

	void setFormatFor(Construct construct, const QTextCharFormat &format);

	QTextCharFormat formatFor(Construct construct) const
	{ return m_formats[construct]; }

protected:
	enum State {
		NormalState = -1,
		InComment,
		InTag
	};

	void highlightBlock(const QString &text);

private:
	QTextCharFormat m_formats[LastConstruct + 1];
};


#endif // HTML_HIGHLIGHTER_H
