

#include <qlistwidget.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qinputdialog.h>

#include "assettemplatesdialog.h"
#include "assettemplatedialog.h"
#include "recordlistmodel.h"
#include "resinerror.h"
#include "blurqt.h"

AssetTemplatesDialog::AssetTemplatesDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	
	ProjectList pl = Project::select().filter( "fkeyProjectStatus", 4 ).sorted( "name" );
	Project none;
	none.setName( "None (Global Templates)" );
	pl.insert( pl.begin(), none );
	mProjectCombo->setColumn( "name" );
	mProjectCombo->setItems( pl );
	
	AssetTypeList atl = AssetType::select().filter( "disabled", 0 ).sorted( "timesheetcategory" );
	mTypeCombo->setColumn( "name" );
	mTypeCombo->setItems( atl );
	
	connect( mProjectCombo,  SIGNAL( currentChanged( const Record & ) ), SLOT( projectChanged( const Record & ) ) );
	connect( mTypeCombo,  SIGNAL( currentChanged( const Record & ) ), SLOT( assetTypeChanged( const Record & ) ) );
	
	connect( mAddButton, SIGNAL( clicked() ),  SLOT( addTemplate() ) );
	connect( mEditButton, SIGNAL( clicked() ), SLOT( editTemplate() ) );
	connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removeTemplate() ) );
	
	connect( mTemplateList, SIGNAL( currentChanged( const Record & ) ), SLOT( slotCurrentChanged( const Record & ) ) );

	RecordListModel * model = new RecordListModel( this );
	model->setColumn( "name" );
	mTemplateList->setModel( model );

	assetTypeChanged( mTypeCombo->current() );
}

void AssetTemplatesDialog::setProject( const Project & project )
{
	mProject = project;
	if( project.isRecord() )
		mProjectCombo->setCurrent( project );
	else
		mProjectCombo->setCurrentIndex( 0 );
	updateTemplates();
}

void AssetTemplatesDialog::setAssetType( const AssetType & assetType )
{
	if( assetType.isRecord() ) {
		mAssetType = assetType;
		mTypeCombo->setCurrent( assetType );
	}
	updateTemplates();
}

void AssetTemplatesDialog::accept()
{
	mRemoved.remove();
	QDialog::accept();
}

void AssetTemplatesDialog::reject()
{
	mAdded.remove();
	QDialog::reject();
}

void AssetTemplatesDialog::projectChanged( const Record & project )
{
	mProject = project;
	updateTemplates();
}

void AssetTemplatesDialog::assetTypeChanged( const Record & assetType )
{
	AssetType at = assetType;
	if( at.isRecord() ) {
		mAssetType = at;
		updateTemplates();
	}
}

void AssetTemplatesDialog::slotCurrentChanged( const Record & r )
{
	LOG_5( "AssetTemplatesDialog::slotCurrentChanged" );
	mTemplate = r;
	if( mTemplate.isValid() ) {
		mEditButton->setEnabled( true );
		mRemoveButton->setEnabled( mTemplate.name() != "Default" );
	} else {
		mEditButton->setEnabled( false );
		mRemoveButton->setEnabled( false );
	}
}

void AssetTemplatesDialog::addTemplate()
{
	bool valid;
	QString name = QInputDialog::getText( this, "Enter Template Name", "Enter Template Name", QLineEdit::Normal, QString(), &valid );
	if( valid ) {
		if( name.isEmpty() ) {
			ResinError::nameEmpty( this, "Template" );
			return;
		}
		if( mTemplates.names().contains( name ) ) {
			ResinError::nameTaken( this, "Template" );
			return;
		}
		AssetTemplate t;
		t.setProject( mProject );
		t.setAssetType( mAssetType );
		t.setName( name );
		t.commit();
		mAdded += t;
		updateTemplates();
	}
}

void AssetTemplatesDialog::removeTemplate()
{
	if( mTemplate.isRecord() ) { //&& mTemplate.name() != "Default" ) {
		if( mAdded.contains( mTemplate ) ) {
			mAdded -= mTemplate;
			mTemplate.remove();
		} else
			mRemoved += mTemplate;
		updateTemplates();
	}
}

void AssetTemplatesDialog::editTemplate()
{
	AssetTemplateDialog * atd = new AssetTemplateDialog( this );
	atd->setTemplate( mTemplate );
	atd->exec();
	delete atd;
}

void AssetTemplatesDialog::updateTemplates()
{
	mTemplates = AssetTemplate::recordsByProjectAndAssetType( mProject, mAssetType );
	mTemplates -= mRemoved;
	mTemplates += mAdded.filter( "fkeyassettype", mAssetType.key() );
	LOG_5( "AssetTemplatesDialog::updateTemplates: " + mTemplates.names().join(",") );
	if( !mProject.isRecord() && !mTemplates.names().contains( "Default" ) ) {
		AssetTemplate at;
		at.setName( "Default" );
		at.setAssetType( mAssetType );
		mTemplates += at;
		mAdded += at;
	} 
	mTemplates = mTemplates.sorted( "name" );
	((RecordListModel*)mTemplateList->model())->setRecordList( mTemplates );
	LOG_5( "AssetTemplatesDialog::updateTemplates: Calling setCurrent.  mTemplates.size() == " + QString::number( mTemplates.size() ) );
	mTemplateList->setCurrent( mTemplates.isEmpty() ? Record() : mTemplates[0] );
}

