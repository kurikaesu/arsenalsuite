
#include <qevent.h>
#include <qicon.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmenu.h>
#include <qpushbutton.h>

#include "expression.h"

#include "filteredit.h"

FilterEdit::FilterEdit(QWidget * parent, Field * matchField, const QString & label)
: QWidget( parent )
, mMatchField(matchField)
, mLabel( 0 )
, mLineEdit( 0 )
, mClearButton( 0 )
, mFilterType( WildCardContainsFilter )
, mCaseSensitive( false )
, mUpdateMode( UpdateOnEnter )
{
	QHBoxLayout * layout = new QHBoxLayout(this);
	layout->setMargin(0);
	mLineEdit = new QLineEdit( this );
	QIcon icon(":/images/locationbar_erase.png");
	mClearButton = new QPushButton( icon, icon.isNull() ? "Clear" : QString(), this );
	mClearButton->setFixedHeight( mLineEdit->sizeHint().height() );
	mClearButton->setAutoDefault( false );
	mLabel = new QLabel( label, this );
	layout->addWidget( mLabel );
	layout->addWidget( mLineEdit );
	layout->addWidget( mClearButton );

	mLineEdit->installEventFilter(this);
	
	connect( mLineEdit, SIGNAL( returnPressed() ), SLOT( slotEmitFilterChanged() ) );
	connect( mClearButton, SIGNAL( clicked() ), SLOT( slotClearText() ) );
}


void FilterEdit::setMatchField( Field * field )
{
	mMatchField = field;
}

void FilterEdit::setUpdateMode( UpdateMode um )
{
	if( um != mUpdateMode ) {
		mUpdateMode = um;
		if( um == UpdateOnEdit )
			connect( mLineEdit, SIGNAL(textChanged(const QString&)), SLOT(slotEmitFilterChanged()) );
		else
			mLineEdit->disconnect( SIGNAL(textEdited(const QString&)), this );
	}
}

Expression FilterEdit::expression() const
{
	Expression ret;
	if( !mMatchField ) return ret;
	
	QString editText( mLineEdit->text() );
	if( editText.startsWith( "sql:" ) )
		ret = Expression::sql(editText.mid(4));
	else if( !editText.isEmpty() ) {
		ret = Expression::createField(mMatchField);
		if( mFilterType == WildCardContainsFilter || mFilterType == WildCardFullMatchFilter ) {
			QString filterString;
			if( mFilterType == WildCardContainsFilter )
				filterString += "%";
			filterString += editText.replace( "_", "\\\\_" ).replace( "*", "%" );
			if( mFilterType == WildCardContainsFilter )
				filterString += "%";
			if( mCaseSensitive )
				ret = ret.like(Expression(filterString));
			else
				ret = ret.ilike(Expression(filterString));
		} else if( mFilterType == RegExFilter )
			ret = ret.regexSearch( editText, mCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive );
	}
	return ret;
}

void FilterEdit::slotEmitFilterChanged()
{
	emit filterChanged( expression() );
}

void FilterEdit::slotClearText()
{
	mLineEdit->setText(QString());
	emit filterChanged(Expression());
}

bool FilterEdit::eventFilter( QObject * o, QEvent * e )
{
	if( o == mLineEdit && e->type() == QEvent::ContextMenu ) {
		QMenu * menu = mLineEdit->createStandardContextMenu();
		QAction * first = menu->actions()[0];
		QAction * cs = new QAction( "Case Sensitive Filter", menu );
		cs->setCheckable( true );
		cs->setChecked( mCaseSensitive );
		menu->insertAction( first, cs );
		menu->insertSeparator( first );
		QAction * wildCardContainsAction = new QAction( "Wild Card Contains Filter", menu );
		wildCardContainsAction->setCheckable( true );
		wildCardContainsAction->setChecked( mFilterType == WildCardContainsFilter );
		menu->insertAction( first, wildCardContainsAction );
		QAction * wildCardMatchAction = new QAction( "Wild Card Full Match Filter", menu );
		wildCardMatchAction->setCheckable( true );
		wildCardMatchAction->setChecked( mFilterType == WildCardFullMatchFilter );
		menu->insertAction( first, wildCardMatchAction );
		QAction * regexAction = new QAction( "Regular Expression Filter", menu );
		regexAction->setCheckable( true );
		regexAction->setChecked( mFilterType == RegExFilter );
		menu->insertAction( first, regexAction );
		menu->insertSeparator( first );
		QAction * result = menu->exec(((QContextMenuEvent*)e)->globalPos());
		bool needRefresh = true;
		if( result == cs )
			mCaseSensitive = !mCaseSensitive;
		else if( result == wildCardContainsAction )
			mFilterType = WildCardContainsFilter;
		else if( result == wildCardMatchAction )
			mFilterType = WildCardFullMatchFilter;
		else if( result == regexAction )
			mFilterType = RegExFilter;
		else
			needRefresh = false;
		if( needRefresh )
			slotEmitFilterChanged();
		delete menu;
		return true;
	}
	return QWidget::eventFilter(o,e);
}
