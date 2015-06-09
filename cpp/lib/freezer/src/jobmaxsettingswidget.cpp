

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

/* $Author: newellm $
 * $LastChangedDate: 2012-04-13 15:32:08 -0700 (Fri, 13 Apr 2012) $
 * $Rev: 12965 $
 * $HeadURL: svn://newellm@svn.blur.com/blur/trunk/cpp/lib/assfreezer/src/jobmaxsettingswidget.cpp $
 */

#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qmessagebox.h>

#include "path.h"
#include "database.h"

#include "recordproxy.h"

#include "host.h"
#include "job.h"
#include "jobmax.h"
#include "jobhistory.h"
#include "jobhistorytype.h"
#include "jobtask.h"
#include "jobservice.h"
#include "service.h"
#include "user.h"

#include "framenthdialog.h"
#include "jobmaxsettingswidget.h"

JobMaxUtils::JobMaxUtils( CustomJobSettingsWidget * cjsw )
: mCJSW( cjsw )
{
}

void JobMaxUtils::clearCache()
{
	mMaxServicesByJob.clear();
}

bool JobMaxUtils::is64Service( JobService js )
{
	return js.service().service().endsWith("_64");
}

QString JobMaxUtils::serviceCounterpartName( Service s )
{
	QString name(s.service());
	if( name.endsWith("_64") ) {
		name.chop(3);
		return name;
	}
	return name + "_64";
}

Service JobMaxUtils::serviceCounterpart( Service s )
{
	return Service::recordByName(serviceCounterpartName(s));
}

bool JobMaxUtils::hasServiceCounterpart( Service s )
{
	return serviceCounterpart(s).isRecord();
}

JobService JobMaxUtils::set64Service( JobService js, bool use64 )
{
	if( is64Service( js ) == use64 ) return js;

	QString oldServiceName = js.service().service();
	Service newService = serviceCounterpart(js.service());
	if( newService.isRecord() )
		js.setService(newService);

	return js;
}

JobService JobMaxUtils::findMaxService( const Job & job )
{
	if( mMaxServicesByJob.contains( job ) ) return mMaxServicesByJob[job];

	JobService maxService;
	JobServiceList jsl = mCJSW->getJobServices(job);
	QRegExp maxre("^MAX\\d");
	foreach( JobService js, jsl )
		if( maxre.indexIn(js.service().service()) == 0 ) {
			maxService = js;
			break;
		}
	mMaxServicesByJob[job] = maxService;
	return maxService;
}

void JobMaxUtils::reset64BitCheckBox( JobList jobs, QCheckBox * cb )
{
	uint validCounterparts = 0, is64Bit = 0;
	foreach( Job j, jobs ) {
		JobService js = findMaxService( j );
		validCounterparts += hasServiceCounterpart( js.service() ) ? 1 : 0;
		is64Bit += is64Service( js ) ? 1 : 0;
	}
	
	LOG_5( "Job count: " + QString::number(jobs.size()) + " Valid counterparts: " + QString::number( validCounterparts ) + " 64 Bit count: " + QString::number(is64Bit) );

	// Show it disabled if at least one of the selected job types support it
	if( validCounterparts > 0 ) {
		if( validCounterparts ==jobs.size() ) {
			cb->setEnabled(true);
			bool ts = is64Bit > 0 && is64Bit < jobs.size();
			cb->setTristate( ts );
			cb->setCheckState( ts ? Qt::PartiallyChecked : (is64Bit > 0 ? Qt::Checked : Qt::Unchecked) );
		} else
			cb->setEnabled(false);
		cb->show();
	} else
		cb->hide();
}

void JobMaxUtils::apply64BitCheckBox( JobList jobs, QCheckBox * cb )
{
	if( !cb->isHidden() && cb->checkState() != Qt::PartiallyChecked ) {
		bool use64 = cb->checkState() == Qt::Checked;
		foreach( JobMax job, jobs )
			mCJSW->applyJobServices( job, set64Service( findMaxService( job ), use64 ) );
	}
}

JobService JobMaxUtils::getService( const Job & job, const QString & service )
{
	JobServiceList jsl = mCJSW->getJobServices(job);
	foreach( JobService js, jsl )
		if( js.service().service() == service )
			return js;
	return JobService();
}

void JobMaxUtils::setService( const Job & job, const QString & service, bool setService )
{
	JobService js = getService(job,service);
	if( js.service().isRecord() == setService ) return;
	if( setService ) {
		js.setJob( job );
		js.setService( Service::recordByName( service ) );
		mCJSW->applyJobServices(job,js);
	} else
		mCJSW->removeJobServices(job,js);

	JobHistory jh;
	jh.setJob( js );
	jh.setMessage( "Job Service List Changed: " + service + (setService ? " added" : " removed") );
	jh.setUser( User::currentUser() );
	jh.setHost( Host::currentHost() );
	jh.setColumnLiteral( "created", "NOW()" );
	jh.setType( JobHistoryType::recordByName( "info" ) );
	jh.commit();
}

void JobMaxUtils::resetServiceCheckBox( JobList jobs, const QString & service, QCheckBox * cb )
{
	uint hasServiceCnt = 0;
	foreach( Job j, jobs )
		hasServiceCnt += getService( j, service ).service().isRecord() ? 1 : 0;
	
	// Show it disabled if at least one of the selected job types support it
	bool ts = hasServiceCnt > 0 && hasServiceCnt < jobs.size();
	cb->setTristate( ts );
	cb->setCheckState( ts ? Qt::PartiallyChecked : (hasServiceCnt > 0 ? Qt::Checked : Qt::Unchecked) );
}

void JobMaxUtils::applyServiceCheckBox( JobList jobs, const QString & service, QCheckBox * cb )
{
	if( cb->checkState() != Qt::PartiallyChecked )
		foreach( JobMax job, jobs )
			setService( job, service, cb->checkState() == Qt::Checked );
}

JobMaxSettingsWidget::JobMaxSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
, JobMaxUtils( this )
, mFrameNthChanges( false )
{
	JobMaxSettingsWidget::setupUi( this );

	mProxy = new RecordProxy( this );

	/* Restart Settings connections */
	connect( CameraCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );
	connect( mWidthSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mHeightSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( xvCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( x2Check, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xaCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xeCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xkCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xdCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xhCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xoCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xfCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xnCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xpCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( xcCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( m64BitCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( mPCPreviewServiceCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( mRealflowRenderkitServiceCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( mOutputPathEdit, SIGNAL( textEdited(const QString&) ), SLOT( settingsChange() ) );
	connect( mBrowseOutputPathButton, SIGNAL( clicked() ), SLOT( browseOutputPath() ) );

	if( mode == JobSettingsWidget::ModifyJobs ) 
		connect( mSetupFrameNthButton, SIGNAL( clicked() ), SLOT( changeFrameNthSettings() ) );
	else
		mSetupFrameNthButton->hide();

	mOutputPathEdit->setProxy( mProxy );
	mHeightSpin->setProxy( mProxy );
	mWidthSpin->setProxy( mProxy );

	xvCheck->setProxy( mProxy );
	x2Check->setProxy( mProxy );
	xaCheck->setProxy( mProxy );
	xeCheck->setProxy( mProxy );
	xkCheck->setProxy( mProxy );
	xdCheck->setProxy( mProxy );
	xhCheck->setProxy( mProxy );
	xoCheck->setProxy( mProxy );
	xfCheck->setProxy( mProxy );
	xnCheck->setProxy( mProxy );
	xpCheck->setProxy( mProxy );
	xcCheck->setProxy( mProxy );
	vCheck->setProxy( mProxy );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobMaxSettingsWidget::~JobMaxSettingsWidget()
{
}

QStringList JobMaxSettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobMaxSettingsWidget::jobTypes()
{
	return (QStringList() << "Max7" << "Max8" << "Max9" << "Max10" << "Max2009" << "Max2010" << "Max");
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

void JobMaxSettingsWidget::browseOutputPath()
{
	QString current = mOutputPathEdit->text();
	QString outputPath = QFileDialog::getSaveFileName( this, "Choose Output Path", QFileInfo(current).path() );
	if( !outputPath.isEmpty() ) {
		outputPath = QFileInfo(outputPath).path() + QDir::separator() + framePathBaseName(outputPath);
		mOutputPathEdit->setText( outputPath );
		settingsChange();
	}
}

void JobMaxSettingsWidget::changeFrameNthSettings()
{
	FrameNthDialog * dialog = new FrameNthDialog( this, mFrameNthStart, mFrameNthEnd, mFrameNth, mFrameNthMode );
	if( dialog->exec() == QDialog::Accepted ) {
		dialog->getSettings( mFrameNthStart, mFrameNthEnd, mFrameNth, mFrameNthMode );
		mFrameNthChanges = true;
		settingsChange();
	}
	delete dialog;
}

void JobMaxSettingsWidget::resetSettings()
{
	clearCache();
	mProxy->setRecordList( mSelectedJobs, false, false );
	bool empty = mSelectedJobs.isEmpty();
	setEnabled( !empty );

	if( !empty ) {
		Job js = mSelectedJobs[0];
		CameraCombo->setEditText( js.getValue( "camera" ).toString() );
		mFrameNthStart = js.getValue("frameStart").toInt();
		mFrameNthEnd = js.getValue("frameEnd").toInt();
		mFrameNth = js.frameNth();
		mFrameNthMode = js.frameNthMode();
	}

	reset64BitCheckBox( mSelectedJobs, m64BitCheck );
	resetServiceCheckBox( mSelectedJobs, "PCPreview", mPCPreviewServiceCheck );
	resetServiceCheckBox( mSelectedJobs, "Realflow_Renderkit", mRealflowRenderkitServiceCheck );

	QList<bool> canModifyOutputPath = unique(JobMaxList(mSelectedJobs).canModifyOutputPaths());
	mOutputPathEdit->setReadOnly( !(canModifyOutputPath.size() == 1 && canModifyOutputPath[0] == true) );
	
	CustomJobSettingsWidget::resetSettings();
}

void JobMaxSettingsWidget::applySettings()
{
	if( mMode == JobSettingsWidget::ModifyJobs && 
		QMessageBox::warning( this, "Warning: This will reset the job",
									"Applying the settings will reset the job.",
		 							QMessageBox::Ok, QMessageBox::Cancel, QMessageBox::NoButton)
		!= QMessageBox::Ok ){
		return;
	}
	// Apply changes from proxied widgets and get modified record list
	mProxy->applyChanges();
	mSelectedJobs = mProxy->records();

	apply64BitCheckBox( mSelectedJobs, m64BitCheck );
	applyServiceCheckBox( mSelectedJobs, "PCPreview", mPCPreviewServiceCheck );
	applyServiceCheckBox( mSelectedJobs, "Realflow_Renderkit", mRealflowRenderkitServiceCheck );
	//mSelectedJobs.setValue( "camera", CameraCombo->currentText() );

	if( mMode == JobSettingsWidget::ModifyJobs ) {
		foreach( JobMax js, mSelectedJobs ) {
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
			if( js.isUpdated( Job::c.OutputPath ) )
				js.setPassOutputPath( true );
			mSelectedJobs.update(js);
		}

		mSelectedJobs.commit();

		Job::updateJobStatuses( mSelectedJobs, "ready", true );

		mFrameNthChanges = false;
	}

	CustomJobSettingsWidget::applySettings();
}

