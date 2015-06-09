

/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author$
 * $LastChangedDate: 2010-02-16 17:20:26 +1100 (Tue, 16 Feb 2010) $
 * $Rev: 9358 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/jobmayasettingswidget.cpp $
 */

#include <qmessagebox.h>

#include "database.h"
#include "recordproxy.h"

#include "jobtask.h"
#include "job.h"
#include "host.h"
#include "user.h"

#include "framenthdialog.h"
#include "jobmayasettingswidget.h"

JobMayaSettingsWidget::JobMayaSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
, mFrameNthChanges( false )
{
	JobMayaSettingsWidget::setupUi( this );

	mProxy = new RecordProxy( this );

	/* Restart Settings connections */
	connect( CameraCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );
	connect( RendererCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );
	connect( mWidthSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mHeightSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mAppendEdit, SIGNAL( textEdited(const QString &) ), SLOT( settingsChange() ) );

	if( mode == JobSettingsWidget::ModifyJobs ) 
		connect( mSetupFrameNthButton, SIGNAL( clicked() ), SLOT( changeFrameNthSettings() ) );
	else
		mSetupFrameNthButton->hide();

	mHeightSpin->setProxy( mProxy );
	mWidthSpin->setProxy( mProxy );
	mAppendEdit->setProxy( mProxy );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobMayaSettingsWidget::~JobMayaSettingsWidget()
{
}

QStringList JobMayaSettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobMayaSettingsWidget::jobTypes()
{
	return (QStringList() << "Maya7" << "Maya8" << "Maya85" << "Maya2008" << "Maya2009" << "Maya2011" << "MentalRay85" << "MentalRay2009" << "MentalRay2011" );
}

template<class T> static QList<T> unique( QList<T> list )
{
	return list.toSet().toList();
}

template<class T> static Qt::CheckState listToCheckState( QList<T> list )
{
	list = unique(list);
	return list.size() == 1 ? (list[0] ? Qt::Checked : Qt::Unchecked) : Qt::PartiallyChecked;
}

template<class T> static void checkBoxApplyMultiple( QCheckBox * cb, QList<T> values )
{
	Qt::CheckState state = listToCheckState(values);
	cb->setTristate( state == Qt::PartiallyChecked );
	cb->setCheckState( state );
}

void JobMayaSettingsWidget::changeFrameNthSettings()
{
	FrameNthDialog * dialog = new FrameNthDialog( this, mFrameNthStart, mFrameNthEnd, mFrameNth, mFrameNthMode );
	if( dialog->exec() == QDialog::Accepted ) {
		dialog->getSettings( mFrameNthStart, mFrameNthEnd, mFrameNth, mFrameNthMode );
		mFrameNthChanges = true;
		settingsChange();
	}
	delete dialog;
}

void JobMayaSettingsWidget::resetSettings()
{
	mProxy->setRecordList( mSelectedJobs, false, false );
	bool empty = mSelectedJobs.isEmpty();
	setEnabled( !empty );

	if( !empty ) {
		Job js = mSelectedJobs[0];
		mFrameNthStart = js.getValue("frameStart").toInt();
		mFrameNthEnd = js.getValue("frameEnd").toInt();
		mFrameNth = js.frameNth();
		mFrameNthMode = js.frameNthMode();
	}

	if( !empty ) {
		Job js = mSelectedJobs[0];
		CameraCombo->setEditText( js.getValue( "camera" ).toString() );
		RendererCombo->setEditText( js.getValue( "renderer" ).toString() );
	}

	CustomJobSettingsWidget::resetSettings();
}

void JobMayaSettingsWidget::applySettings()
{
	if(
		QMessageBox::warning( this, "Warning: This will reset the job",
									"Applying the settings will reset the job.",
		 							QMessageBox::Ok, QMessageBox::Cancel, QMessageBox::NoButton)
		!= QMessageBox::Ok ){
		return;
	}
	// Apply changes from proxied widgets and get modified record list
	mProxy->applyChanges();
	mSelectedJobs = mProxy->records();

	if( mMode == JobSettingsWidget::ModifyJobs ) {
		foreach( Job js, mSelectedJobs ) {
			if( mFrameNthChanges ) {
				int start = js.getValue( "frameStart" ).toInt(), end = js.getValue( "frameEnd" ).toInt();
				if( js.frameNth() != mFrameNth || start != mFrameNthStart || end != mFrameNthEnd || mFrameNthMode != js.frameNthMode() ) {
					QList<int> frames;
					for( int i=mFrameNthStart; i <= mFrameNthEnd; i++ )
						if( ((i-mFrameNthStart) % mFrameNth) == 0 )
							frames << i;
					Database::current()->beginTransaction();
					js.changeFrameRange(frames,JobOutput(),/*changeCancelledToNew=*/true);
					js.setFrameNth( mFrameNth ).setFrameNthMode( mFrameNthMode );
					js.commit();
					Database::current()->commitTransaction();
				}
			}
		}

		mSelectedJobs.commit();

		Job::updateJobStatuses( mSelectedJobs, "new", true );
	}

	CustomJobSettingsWidget::applySettings();
}


