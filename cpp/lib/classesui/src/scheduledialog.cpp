
#include <qcheckbox.h>
#include <qcombobox.h>

#include "asset.h"
#include "elementuser.h"
#include "schedule.h"
#include "scheduledialog.h"
#include "shot.h"
#include "userrole.h"
#include "database.h"
#include "task.h"

#include "datechooserwidget.h"
#include "elementmodel.h"
#include "timeentrydialog.h"

ScheduleDialog::ScheduleDialog( QWidget * parent )
: QDialog( parent )
, mAssetModel( 0 )
, mDisableUpdates( false )
{
	setupUi( this );

	mAssetModel = new ElementModel( mAssetTree );
	mAssetModel->setSecondColumnIsLocation( true );
	mAssetModel->setAutoSort( true );
	mAssetTree->setModel( mAssetModel );

	mUserModel = new ElementModel( mUserCombo );
	mUserModel->setAutoSort( true );
	mUserCombo->setModel( mUserModel );
	mUserCombo->setModelColumn( 0 );

//	connect( mCalendarButton, SIGNAL( clicked() ), SLOT( showCalendar() ) );
	connect( mProjectCombo, SIGNAL( activated( const QString & ) ), SLOT( projectSelected( const QString & ) ) );
	connect( mTypeCombo, SIGNAL( currentChanged( const Record & ) ), SLOT( assetTypeChanged( const Record & ) ) );
	connect( mTypeFilterCheck, SIGNAL( toggled( bool ) ), SLOT( setUseTypeFilter( bool ) ) );
	connect( mTypeFilterAssetsCheck, SIGNAL( toggled( bool ) ), SLOT( setUseAssetsTypeFilter( bool ) ) );

	connect( mUserCombo, SIGNAL( currentChanged( const Record & ) ), SLOT( employeeSelected( const Record & ) ) );

	connect( mChooseStartDateButton, SIGNAL( clicked() ), SLOT( chooseStartDate() ) );
	connect( mChooseEndDateButton, SIGNAL( clicked() ), SLOT( chooseEndDate() ) );

	QDate d( QDate::currentDate() );
	setDateRange( d, d );
	ProjectList pl = Project::select().filter( "fkeyProjectStatus", 4 ).sorted( "name" );
	mProject = pl[0];
	mProjectCombo->addItems( pl.names() );

	mTypeCombo->setTagFilters( QStringList() << "schedule" );
	mAssetType = mTypeCombo->current();

	updateUsers();

	mDateStartEdit->setMaximumDate( QDate::currentDate() );
	mDateEndEdit->setMinimumDate( QDate::currentDate() );
}

void ScheduleDialog::accept()
{
	RecordList assetSelection = mAssetTree->selection();
	Element e = assetSelection.isEmpty() ? Element(mProject) : Element(assetSelection[0]);

	if( mSchedule.isRecord() ) {
		Database::current()->beginTransaction( "Modify Schedule" );
		mSchedule.setDuration( Interval().addHours(mHoursSpin->value()) );
		mSchedule.setElement( e );
		mSchedule.setAssetType( mAssetType );
		mSchedule.setUser( mEmployee );
		mSchedule.setDate( mDateStartEdit->date() );
		mSchedule.setCreatedByUser( User::currentUser() );
		mSchedule.commit();
		Database::current()->commitTransaction();
	} else {
		Database::current()->beginTransaction( "Create Schedule" );

		ScheduleList toCommit;
		QDate start = mDateStartEdit->date();
		QDate end = mDateEndEdit->date();
		while( start <= end ) {
			Schedule s;
			s.setUser( mEmployee );
			s.setElement( e );
			s.setAssetType( mAssetType );
			s.setDate( start );
			s.setDuration( Interval().addHours(mHoursSpin->value()) );
			s.setCreatedByUser( User::currentUser() );
			toCommit += s;
			start = start.addDays( 1 );
		}
		toCommit.commit();
	
		if( !e.users().contains( mEmployee ) ) {
			ElementUser eu;
			eu.setElement( e );
			eu.setUser( mEmployee );
			eu.commit();
		}
	
		Database::current()->commitTransaction();
	}
	QDialog::accept();
}

void ScheduleDialog::setElement( const Element & e, const AssetType & at )
{
	if( e.isRecord() ) {
		mDisableUpdates = true;
		setProject( e.project() );
		if( at.isRecord() )
			setAssetType( at );
		if( at != e.assetType() && mTypeFilterAssetsCheck->isChecked() )
			setUseAssetsTypeFilter( false );
		mAssetFilterEdit->setText( e.displayName(true) );
		mDisableUpdates = false;
		updateAssets();
		mAssetTree->setSelection( e );
	}
}

void ScheduleDialog::setAssetType( const AssetType & tt )
{
	if( tt.isRecord() ) {
		mAssetType = tt;
		mTypeCombo->setCurrent( tt );
		updateUsers();
	}
}

void ScheduleDialog::setEmployee( const Employee & e )
{
	if( e.isRecord() ) {
		mEmployee = e;
		if( mTypeFilterCheck->isChecked() && !mUserModel->rootList().contains( e ) )
			setUseTypeFilter( false );
		mUserCombo->setCurrent( e );
	}
}

void ScheduleDialog::setProject( const Project & project )
{
	if( project.isRecord() ) {
		mProject = project;
		int idx = mProjectCombo->findText( project.name() );
		if( idx >= 0 )
			mProjectCombo->setCurrentIndex( idx );
		updateAssets();
	}
}

void ScheduleDialog::setDateRange( const QDate & start, const QDate & end )
{
	if( end < start ) return;
	startDateChanged( start );
	endDateChanged( end );
	mDateStartEdit->setDate( start );
	mDateEndEdit->setDate( end );
}

void ScheduleDialog::chooseStartDate()
{
	setDateRange( DateChooserDialog::getDate(this,mDateStartEdit->date()), mDateEndEdit->date() );
}

void ScheduleDialog::chooseEndDate()
{
	setDateRange( mDateStartEdit->date(), DateChooserDialog::getDate(this,mDateEndEdit->date()) );
}

void ScheduleDialog::startDateChanged( const QDate & date )
{
	mDateEndEdit->setMinimumDate( date );
}

void ScheduleDialog::endDateChanged( const QDate & date )
{
	mDateStartEdit->setMaximumDate( date );
}

void ScheduleDialog::projectSelected( const QString & project )
{
	Project p = Project::recordByName( project );
	if( p.isRecord() ) {
		mProject = p;
		updateAssets();
	}
}

void ScheduleDialog::employeeSelected( const Record & employee )
{
	mEmployee = employee;
}

void ScheduleDialog::setUseTypeFilter( bool tf )
{
	mTypeFilterCheck->setChecked( tf );
	updateUsers();
}

void ScheduleDialog::setUseAssetsTypeFilter( bool atf )
{
	mTypeFilterAssetsCheck->setChecked( atf );
	updateAssets();
}

void ScheduleDialog::assetTypeChanged( const Record & assetType )
{
	mAssetType = assetType;
	updateAssets();
	updateUsers();
}

void ScheduleDialog::updateAssets()
{
	if( mDisableUpdates ) return;
	bool en = mAssetType.isRecord() && mProject.isRecord();
	mAssetTree->setEnabled( en );
	mAssets.clear();
	if( !en ) return;
	mAssets = Element::recordsByProject( mProject );
	if( mTypeFilterAssetsCheck->isChecked() )
		mAssets = mAssets.filter( "assettype", mAssetType.key() );
	//LOG_5( "TimeEntryDialog::updateAssets: Got " + QString::number( list.size() ) + " assets" );
/*	st_foreach( ElementIter, it, list ) {
		Element e(*it);
		QVariant v = e.getValue( "allowTime" );
		if( (v.isNull() && e.children().isEmpty()) || (!v.isNull() && v.toBool()) )
			mAssets += e;
	} */
	RecordList sel = mAssetTree->selection();
	mAssetModel->setRootList( TimeEntryDialog::filterAssets( mAssets, mAssetFilterEdit->text() ) );
	if( !sel.isEmpty() ) {
		mAssetTree->setSelection( sel );
		mAssetTree->scrollTo( sel );
	}
}

void ScheduleDialog::updateUsers()
{
	if( mDisableUpdates ) return;
	Record cur = mUserCombo->current();
	EmployeeList el;
	if( mTypeFilterCheck->isChecked() && mAssetType.isRecord() ) {
		el = UserRole::recordsByAssetType( mAssetType ).users();
	} else
		el = Employee::select();
	el = el.filter( "disabled", 0 );
	mUserCombo->setItems( el );
	if( cur.isRecord() )
		mUserCombo->setCurrent( cur );
}

void ScheduleDialog::setSchedule( const Schedule & schedule )
{
	mSchedule = schedule;
	setWindowTitle( "Modify Schedule..." );
	mDateEndEdit->setEnabled( false );
	setDateRange( schedule.date(), schedule.date() );
//	setAssetType( schedule.assetType() );
	setElement( schedule.element(), schedule.assetType() );
	setEmployee( schedule.user() );
	mHoursSpin->setValue( schedule.duration().hours() );
}


