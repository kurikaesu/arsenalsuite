
class HtmlHighlighter : QSyntaxHighlighter
{
%TypeHeaderCode
#include "htmlhighlighter.h"
%End
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

	QTextCharFormat formatFor(Construct construct) const;

protected:
	enum State {
		NormalState = -1,
		InComment,
		InTag
	};

	void highlightBlock(const QString &text);

};