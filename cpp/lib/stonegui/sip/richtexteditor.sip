
class RichTextEditor : QTextEdit
{
%TypeHeaderCode
#include "richtexteditor.h"
%End
public:
	RichTextEditor(QWidget *parent = 0);
	void setDefaultFont(const QFont &font);

	QToolBar *createToolBar(QWidget *parent = 0);

public slots:
	void setFontBold(bool b);
	void setFontPointSize(double);
	void setText(const QString &text);
	QString text(Qt::TextFormat format) const;

signals:
	void stateChanged();
};

class RichTextEditorDialog : QDialog
{
%TypeHeaderCode
#include "richtexteditor.h"
%End
public:
	RichTextEditorDialog(QWidget *parent = 0);
	~RichTextEditorDialog();

	int showDialog();
	void setDefaultFont(const QFont &font);
	void setText(const QString &text);
	QString text(Qt::TextFormat format = Qt::AutoText) const;

private slots:
	void tabIndexChanged(int newIndex);
	void richTextChanged();
	void sourceChanged();
};
