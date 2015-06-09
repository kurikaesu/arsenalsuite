
#include <qicon.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>

#include "jobfilteredit.h"

JobFilterEdit::JobFilterEdit(QWidget * parent)
: QWidget( parent )
, mLineEdit( 0 )
, mClearButton( 0 )
{
	QHBoxLayout * layout = new QHBoxLayout(this);
	layout->setMargin(0);
	mLineEdit = new QLineEdit( this );
	QIcon icon(":/images/locationbar_erase.png");
	mClearButton = new QPushButton( icon, icon.isNull() ? "Clear" : QString(), this );
	mClearButton->setFixedHeight( mLineEdit->sizeHint().height() );
	layout->addWidget( new QLabel( "Job Name Filter:", this ) );
	layout->addWidget( mLineEdit );
	layout->addWidget( mClearButton );

	connect( mLineEdit, SIGNAL( returnPressed() ), SLOT( slotTextEdited() ) );
	connect( mClearButton, SIGNAL( clicked() ), SLOT( slotClearText() ) );
}

QString JobFilterEdit::sqlFilter() const
{
	QString ret, editText( mLineEdit->text() );
	if( editText.startsWith( "sql:" ) )
		ret = editText.mid(4);
	else if( !editText.isEmpty() )
		ret = "job~*'" + editText + "'";
	return ret;
}

void JobFilterEdit::slotTextEdited()
{
	emit filterChanged( sqlFilter() );
}

void JobFilterEdit::slotClearText()
{
	QString ns;
	mLineEdit->setText(ns);
	emit filterChanged(ns);
}

