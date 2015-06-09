/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtooltip.h>

#include "blurqt.h"
#include "database.h"

#include "asset.h"
#include "assettype.h"
#include "elementstatus.h"
#include "elementuser.h"
#include "employee.h"
#include "project.h"
#include "shot.h"
#include "schedule.h"
#include "task.h"

#include "recordlistview.h"

#include "datechooserwidget.h"
#include "elementmodel.h"
#include "timeentrydialog.h"


TimeEntryDialog::TimeEntryDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );

	mAssetModel = new ElementModel( mAssetTree );
	mAssetModel->setSecondColumnIsLocation( true );
	mAssetTree->setModel( mAssetModel );

//	mCalendarButton->hide();

	connect( mChooseStartDateButton, SIGNAL( clicked() ), SLOT( chooseStartDate() ) );
	connect( mChooseEndDateButton, SIGNAL( clicked() ), SLOT( chooseEndDate() ) );
	connect( mProjectCombo, SIGNAL( activated( const QString & ) ), SLOT( projectSelected( const QString & ) ) );
	connect( mTypeCombo, SIGNAL( currentChanged( const Record & ) ), SLOT( assetTypeChanged( const Record & ) ) );
	connect( mAssetFilterEdit, SIGNAL( textChanged( const QString & ) ), SLOT( slotFilterAssets() ) );
	
	setDateRange( QDate::currentDate() );
	ProjectList pl = Project::select().filter( "fkeyProjectStatus", 4 ).sorted( "name" );
	mProject = pl[0];
	mProjectCombo->addItems( pl.names() );

	mTypeCombo->setTagFilters( QStringList() << "timesheet" );
	mTypeCombo->setShowFirst( User::currentUser().roles() );
	mAssetType = mTypeCombo->current();
	mUser = User::currentUser();

	mForcedProjectCategories += "Vacation";
	mForcedProjectCategories += "Sick";
	mForcedProjectCategories += "Comp Time";
	mForcedProjectCategories += "Unpaid Leave";
	mForcedProjectCategories += "Holidays";

	mVirtualProject = Project::recordByName( "_virtual_project" );
}

void TimeEntryDialog::setupSuggestedTimeSheet()
{
	VarList v;
	v += QDate::currentDate();
	v += User::currentUser().key();
	ScheduleList sl = Schedule::select( "date = ? and fkeyuser = ?", v );
	if( !sl.isEmpty() ) {
		Schedule s = sl[0];
		setElementList( s.element() );
	} else {
		TimeSheetList recent = TimeSheet::select( "fkeyemployee=? order by dateTime desc limit 1", VarList() += User::currentUser().key() );
		if( !recent.isEmpty() ) {
			TimeSheet ts = recent[0];
			setProject( ts.project() );
			setAssetType( ts.assetType() );
			setElementList( ts.element() );
		}
	}
}

void TimeEntryDialog::projectSelected( const QString & pn )
{
	mProject = Project::recordByName( pn );
	updateAssets();
}

void TimeEntryDialog::assetTypeChanged( const Record & at )
{
	setAssetType( at );
}

void TimeEntryDialog::setDateRange( const QDate & start, const QDate & end )
{
	mStartDateEdit->setDate( start );
	mEndDateEdit->setDate( (!end.isValid() || end < start) ? start : end );
}

void TimeEntryDialog::chooseStartDate()
{
	setDateRange( DateChooserDialog::getDate(this,mStartDateEdit->date()), mEndDateEdit->date() );
}

void TimeEntryDialog::chooseEndDate()
{
	setDateRange( mStartDateEdit->date(), DateChooserDialog::getDate(this,mEndDateEdit->date()) );
}

void TimeEntryDialog::setTimeSheet( const TimeSheet & ts )
{
	mTimeSheet = ts;
	mUser = mTimeSheet.user();
	setDateRange( ts.dateTime().date() );
	setProject( ts.project() );
	setAssetType( ts.assetType() );
	mHoursSpin->setValue( ts.scheduledHour() );
	mCommentEdit->setPlainText( ts.comment() );
	setElementList( ts.element() );
}

void TimeEntryDialog::setProject( const Project & p )
{
	if( p.isRecord() && p != mProject ) {
		mProjectCombo->setCurrentIndex( mProjectCombo->findText( p.name() ) );
		mProject = p;
		updateAssets();
	}
}

void TimeEntryDialog::setElementList( ElementList list )
{
	if( list.isEmpty() ) return;
	// Go through the element list and make sure that
	// each element is in the same project and has
	// the same asset type
	ElementList valid;
	ElementList invalid;
	AssetType type;
	Project p;
	foreach( Element e, list ) {
		if( p.isRecord() && e.project() != p ) continue;
		QVariant v = e.getValue( "allowTime" );
		if( v.toBool() || (v.isNull() && (e.children().size()==0) ) ) {
			if( !type.isRecord() ) {
				type = e.assetType();
				p = e.project();
			} else if( e.assetType() != type )
				continue;
			if( !valid.contains( e ) ) valid += e;
		} else
			invalid += e;
	}

	mAssetTree->setSelection( RecordList() );

	if( valid.isEmpty() ) {
		if( !invalid.isEmpty() ) {
			setProject( invalid[0].project() );
			QStringList sl;
			foreach( Element e, invalid )
				sl += e.displayName(true);
			mAssetFilterEdit->setText( sl.join(",") );
		}
	} else {
		Element first = valid[0];
		QStringList sl;
		foreach( Element e, valid )
			sl += e.displayName(true);
		mAssetFilterEdit->setText( sl.join(",") );
		if( first.isRecord() ) {
			setProject( first.project() );
			setAssetType( type );
			mAssetTree->setSelection( valid );
			mAssetTree->scrollTo( first );
		}
	}
}

void TimeEntryDialog::setAssetType( const AssetType & at )
{
	if( at.isRecord() && at != mAssetType ) {
		mTypeCombo->setCurrent( at );
		mAssetType = at;
		mForceVirtualProject = mForcedProjectCategories.contains( at.name() );
		mProjectCombo->setEnabled( !mForceVirtualProject );
		if( mForceVirtualProject )
			setProject( mVirtualProject );
		updateAssets();
	}
}

void TimeEntryDialog::updateAssets()
{
	bool en = mAssetType.isRecord() && mProject.isRecord();
	mAssetTree->setEnabled( en );
	mAssets.clear();
	if( !en ) return;
	ElementList list = Element::recordsByProject( mProject ).filter( "assettype", mAssetType.key() );
	//LOG_5( "TimeEntryDialog::updateAssets: Got " + QString::number( list.size() ) + " assets" );
	foreach( Element e, list ) {
		QVariant v = e.getValue( "allowTime" );
		if( (v.isNull() && e.children().isEmpty()) || (!v.isNull() && v.toBool()) )
			mAssets += e;
	}
	slotFilterAssets();
}

void TimeEntryDialog::slotFilterAssets()
{
	RecordList sel = mAssetTree->selection();
	mAssetModel->setRootList( filterAssets( mAssets, mAssetFilterEdit->text() ) );
	if( !sel.isEmpty() ) {
		mAssetTree->setSelection( sel );
		mAssetTree->scrollTo( sel );
	}
	if( mForceVirtualProject && mAssets.size() == 1 ) {
		mAssetTree->setSelection( mAssets );
		mAssetTree->setEnabled( false );
	} else
		mAssetTree->setEnabled( true );
}

ElementList TimeEntryDialog::filterAssets( ElementList assets, const QString & filter )
{
	ElementList toShow;
	QStringList filters = filter.split(',');
	foreach( Element e, assets ) {
		bool add = false;
		if( filters.isEmpty() )
			add = true;
		else {
			QString dn = e.displayName(true);
			foreach( QString f, filters )
				if( !dn.isEmpty() && dn.contains( f, Qt::CaseInsensitive ) ) {
					add = true;
					break;
				}
		}
		if( add )
			toShow += e;
	}
	
	// Dont filter at all if there are no matches.
	if( !filter.isEmpty() && toShow.isEmpty() )
		return assets;
	return toShow;
}

void TimeEntryDialog::accept()
{
	ElementList assets = mAssetTree->selection();

	// If the element isn't set, set it to the project
	if( assets.isEmpty() )
		assets += mProject;
	
	#define PER_TASK( x ) (x*time_mul)
	float time_mul = 1.0 / (float)assets.size();

	QDate date = mStartDateEdit->date();
	QDate end = mEndDateEdit->date();
	for( ; date <= end; date = date.addDays(1) ) {
		foreach( Element e, assets ) {
			// Find the task for this element and task type
			// create it if it doesn't exist
			Element t = e;
			if( e.isRecord() && mAssetType != e.assetType() ) {
				ElementList tl = e.children();
				foreach( Element e2, tl ) {
					if( e2.isTask() && e2.assetType() == mAssetType ) {
						t = e2;
						break;
					}
				}
				if( !t.isRecord() ) {
					t.setParent( e );
					t.setProject( mProject );
					t.setAssetType( mAssetType );
					t.setName( mAssetType.name() );
					t.setElementType( Task::type() );
					t.commit();
				}
			}
			
			// Associate the user with the task
			// if the task exists
			if( t.isRecord() ) {
				ElementUserList ul = ElementUser::recordsByElement( t );
				if( !ul.users().contains( mUser ) ) {
					ElementUser tu;
					tu.setElement( t );
					tu.setUser( mUser );
					tu.commit();
				}
			}
	
			// Finally, create the timesheet, we will use the existing record
			// for the first one, then create new records for each subsequent
			// timesheet
			TimeSheet ts = mTimeSheet;
			mTimeSheet = TimeSheet();
			ts.setUser( mUser );
			ts.setDateTime( QDateTime( date ) );
			ts.setDateTimeSubmitted( QDateTime::currentDateTime() );
			ts.setScheduledHour( PER_TASK(mHoursSpin->value()) );
			ts.setUnscheduledHour( 0 );
			ts.setElement( t );
			ts.setProject( mProject );
			ts.setAssetType( mAssetType );
			ts.setComment( mCommentEdit->toPlainText() );
			ts.commit();
		}
	}
	
	// Add this type as a role for the user
	// This function ensures there are not duplicates
	mUser.addRole( mAssetType );
	
	QDialog::accept();
}

bool TimeEntryDialog::recordTimeSheet( QWidget * parent, ElementList elements, const QDate & start, const AssetType & at, const QDate & end )
{
	Database::current()->beginTransaction( "Record Time Sheet" );
	TimeEntryDialog * ted = new TimeEntryDialog( parent );
	ted->setDateRange( start, end );
	ted->setAssetType( at );
	ted->setElementList( elements );
	if( elements.isEmpty() )
		ted->setupSuggestedTimeSheet();
	bool ret = ted->exec() == QDialog::Accepted;
	delete ted;
	Database::current()->commitTransaction();
	return ret;
}
