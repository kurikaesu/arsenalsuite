
#include <qtimer.h>

#include "projectcombo.h"

ProjectCombo::ProjectCombo( QWidget * parent )
: RecordCombo( parent )
, mShowSpecialItem( true )
, mSpecialItemText( "All" )
, mRefreshRequested( false )
{
	connect( this, SIGNAL( currentIndexChanged( int ) ), SLOT( indexChanged( int ) ) );
	mModel = new RecordListModel( this );
	mModel->setColumn( "name" );
	setModel( mModel );
	connect( Project::table(), SIGNAL( added( RecordList ) ), SLOT( refresh() ) );
	connect( Project::table(), SIGNAL( removed( RecordList ) ), SLOT( refresh() ) );
	connect( Project::table(), SIGNAL( updated( Record, Record ) ), SLOT( refresh() ) );
	refresh();
}

Project ProjectCombo::project() const
{
	return mCurrent;
}

void ProjectCombo::setProject( const Project & pt )
{
	if( mRefreshRequested ) doRefresh();
	int idx = mModel->recordList().findIndex( pt );
	if( idx >= 0 ) {
		mCurrent = pt;
		setCurrentIndex( idx );
	}
}

ProjectStatusList ProjectCombo::statusFilters() const
{
	return mStatusFilters;
}

bool ProjectCombo::showingSpecialItem() const
{
	return mShowSpecialItem;
}

void ProjectCombo::setShowSpecialItem( bool sai )
{
	if( sai != mShowSpecialItem ) {
		mShowSpecialItem = sai;
		refresh();
	}
}

void ProjectCombo::setSpecialItemText( const QString & text )
{
	mSpecialItemText = text;
}

QString ProjectCombo::specialItemText() const
{
	return mSpecialItemText;
}

void ProjectCombo::setStatusFilters( ProjectStatusList filters )
{
	mStatusFilters = filters;
	refresh();
}

void ProjectCombo::indexChanged( int i )
{
	ProjectList ptl( mModel->recordList() );
	if( i < 0 || i >= (int)ptl.size() ) return;
	mCurrent = ptl[i];
	emit projectChanged( mCurrent );
}

void ProjectCombo::refresh()
{
	if( !mRefreshRequested ) {
		QTimer::singleShot(0,this,SLOT(doRefresh()));
		mRefreshRequested = true;
	}
}

void ProjectCombo::doRefresh()
{
	mRefreshRequested = false;

	Project cur = mCurrent;
	clear();
	mCurrent = Project();

	// Filter out templates, all real projects have non null fkeyprojectstatus
	QString query("fkeyprojectstatus IS NOT NULL");
	if( !mStatusFilters.isEmpty() )
		query += " AND fkeyprojectstatus IN (" + mStatusFilters.keyString() + ")";
	
	ProjectList ptl = Project::select(query).sorted( "name" );

	if( mShowSpecialItem ) {
		Project p;
		p.setName( mSpecialItemText );
		ptl.insert( ptl.begin(), p );
	}

	mModel->setRecordList( ptl );

	if( ptl.contains( cur ) ) {
		setCurrentIndex( ptl.findIndex( cur ) );
		mCurrent = cur;
	} else if( !ptl.isEmpty() ) {
		setCurrentIndex( 0 );
		mCurrent = ptl[0];
	}
}

