
#include "pathtemplatecombo.h"

PathTemplateCombo::PathTemplateCombo( QWidget * parent )
: QComboBox( parent )
{
	connect( this, SIGNAL( activated( int ) ), SLOT( indexActivated( int ) ) );
	mModel = new RecordListModel( this );
	mModel->setColumn( "name" );
	setModel( mModel );
	connect( PathTemplate::table(), SIGNAL( added( RecordList ) ), SLOT( refresh() ) );
	connect( PathTemplate::table(), SIGNAL( removed( RecordList ) ), SLOT( refresh() ) );
	connect( PathTemplate::table(), SIGNAL( updated( Record, Record ) ), SLOT( refresh() ) );
	refresh();
}

PathTemplate PathTemplateCombo::pathTemplate()
{
	return mCurrent;
}

void PathTemplateCombo::setPathTemplate( const PathTemplate & pt )
{
	int idx = mModel->recordList().findIndex( pt );
	if( idx >= 0 ) {
		mCurrent = pt;
		setCurrentIndex( idx );
	}
}

void PathTemplateCombo::indexActivated( int i )
{
	PathTemplateList ptl( mModel->recordList() );
	if( i < 0 || i >= (int)ptl.size() ) return;
	mCurrent = ptl[i];
	emit templateChanged( mCurrent );
}

void PathTemplateCombo::refresh()
{
	PathTemplate cur = mCurrent;
	clear();
	mCurrent = PathTemplate();

	PathTemplateList ptl = PathTemplate::select();
	ptl = ptl.sorted( "name" );

	PathTemplate none;
	none.setName( "None" );
	ptl.insert( ptl.begin(), none );

	mModel->setRecordList( ptl );

	if( ptl.contains( cur ) ) {
		setCurrentIndex( ptl.findIndex( cur ) );
		mCurrent = cur;
	} else if( !ptl.isEmpty() ) {
		setCurrentIndex( 0 );
		mCurrent = ptl[0];
	}
}

