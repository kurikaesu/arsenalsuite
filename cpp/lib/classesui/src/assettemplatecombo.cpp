

#include "assettemplatecombo.h"

AssetTemplateCombo::AssetTemplateCombo( QWidget * parent )
: QComboBox( parent )
{
	connect( this, SIGNAL( activated( int ) ), SLOT( indexActivated( int ) ) );
	mModel = new RecordListModel( this );
	mModel->setColumn( "name" );
	setModel( mModel );
	connect( AssetTemplate::table(), SIGNAL( added( RecordList ) ), SLOT( refresh() ) );
	connect( AssetTemplate::table(), SIGNAL( removed( RecordList ) ), SLOT( refresh() ) );
	connect( AssetTemplate::table(), SIGNAL( updated( Record, Record ) ), SLOT( refresh() ) );
}

void AssetTemplateCombo::setAssetType( const AssetType & at )
{
	if( !at.isRecord() || at == mAssetType ) return;
	mAssetType = at;
	refresh();
}

void AssetTemplateCombo::setProject( const Project & p )
{
	if( mProject == p ) return;
	mProject = p;
	refresh();
}

AssetType AssetTemplateCombo::assetType()
{
	return mAssetType;
}

Project AssetTemplateCombo::project()
{
	return mProject;
}

AssetTemplate AssetTemplateCombo::assetTemplate()
{
	return mCurrent;
}

void AssetTemplateCombo::setAssetTemplate( const AssetTemplate & at )
{
	int idx = mModel->recordList().findIndex( at );
	if( idx >= 0 ) {
		setCurrentIndex( idx );
		mCurrent = at;
	}
}

void AssetTemplateCombo::indexActivated( int i )
{
	AssetTemplateList atl( mModel->recordList() );
	if( i < 0 || i >= (int)atl.size() ) return;
	mCurrent = atl[i];
	emit templateChanged( mCurrent );
}

void AssetTemplateCombo::refresh()
{
	AssetTemplate cur = mCurrent;

	clear();
	mCurrent = AssetTemplate();

	if( !mAssetType.isRecord() ) return;

	AssetTemplateList atl;
	// Gather the templates
	// Global templates
	atl = AssetTemplate::recordsByProjectAndAssetType( Project(), mAssetType );
	// Project templates
	if( mProject.isRecord() )
		atl += AssetTemplate::recordsByProjectAndAssetType( mProject, mAssetType );

	// Sort the templates
	atl = atl.sorted( "name" );

	// Move 'Default' Template to beginning of the list
	AssetTemplateList tmp = atl.filter( "name", "Default" );
	if( tmp.size() > 1 ) {
		atl -= tmp;
		atl.insert( atl.begin(), tmp[0] );
	}

	AssetTemplate none;
	none.setName( "None" );

	atl.insert( atl.begin(), none );

	mModel->setRecordList( atl );

	if( atl.contains( cur ) ) {
		setCurrentIndex( atl.findIndex( cur ) );
		mCurrent = cur;
	} else if( !atl.isEmpty() ) {
		setCurrentIndex( 0 );
		mCurrent = atl[0];
	}
}

