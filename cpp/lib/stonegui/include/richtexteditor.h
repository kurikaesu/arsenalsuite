
#ifndef RICH_TEXT_EDITOR_H
#define RICH_TEXT_EDITOR_H

#include <QtCore/QPointer>

#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QTextEdit>
#include <QtGui/QToolBar>

#include <qaction.h>

#include "stonegui.h"

#include "ui_addlinkdialog.h"

class QTabWidget;
class QToolBar;
class RichTextEditor;

class STONEGUI_EXPORT HtmlTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    HtmlTextEdit(QWidget *parent = 0);

    void contextMenuEvent(QContextMenuEvent *event);

private slots:
    void actionTriggered(QAction *action);
};

class STONEGUI_EXPORT ColorAction : public QAction
{
Q_OBJECT

public:
    ColorAction(QObject *parent);

    const QColor& color() const { return m_color; }
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

private slots:
    void chooseColor();

private:
    QColor m_color;
};

class STONEGUI_EXPORT RichTextEditorToolBar : public QToolBar
{
Q_OBJECT
public:
    RichTextEditorToolBar(RichTextEditor *editor,
                          QWidget *parent = 0);

public slots:
    void updateActions();

private slots:
    void alignmentActionTriggered(QAction *action);
    void sizeInputActivated(const QString &size);
    void colorChanged(const QColor &color);
    void setVAlignSuper(bool super);
    void setVAlignSub(bool sub);
    void insertLink();
    void insertImage();

private:
    QAction *m_bold_action;
    QAction *m_italic_action;
    QAction *m_underline_action;
    QAction *m_valign_sup_action;
    QAction *m_valign_sub_action;
    QAction *m_align_left_action;
    QAction *m_align_center_action;
    QAction *m_align_right_action;
    QAction *m_align_justify_action;
    QAction *m_link_action;
    QAction *m_image_action;
    ColorAction *m_color_action;
    QComboBox *m_font_size_input;

    QPointer<RichTextEditor> m_editor;
};

class STONEGUI_EXPORT RichTextEditor : public QTextEdit
{
Q_OBJECT
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

class STONEGUI_EXPORT AddLinkDialog : public QDialog
{
Q_OBJECT

public:
    AddLinkDialog(RichTextEditor *editor, QWidget *parent = 0);
    ~AddLinkDialog();

    int showDialog();

public slots:
    void accept();

private:
    RichTextEditor *m_editor;
    Ui::AddLinkDialog *m_ui;
};

class STONEGUI_EXPORT RichTextEditorDialog : public QDialog
{
Q_OBJECT
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

private:
	enum TabIndex { RichTextIndex, SourceIndex };
	enum State { Clean, RichTextChanged, SourceChanged };
	RichTextEditor *m_editor;
	QTextEdit      *m_text_edit;
	QTabWidget     *m_tab_widget;
	State m_state;
};

#endif // RICH_TEXT_EDITOR_H
