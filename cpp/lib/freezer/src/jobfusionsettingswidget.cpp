

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
 * $LastChangedDate: 2008-02-04 11:45:25 -0800 (Mon, 04 Feb 2008) $
 * $Rev: 5578 $
 * $HeadURL: svn://newellm@ocelot/blur/trunk/cpp/lib/assfreezer/src/jobfusionsettingswidget.cpp $
 */

#include <qfileinfo.h>
#include <qmessagebox.h>

#include "jobfusion.h"
#include "jobfusionvideomaker.h"
#include "jobhistory.h"
#include "jobhistorytype.h"
#include "job.h"
#include "jobservice.h"
#include "host.h"
#include "service.h"
#include "user.h"

#include "jobfusionsettingswidget.h"

static JobService findFusionService( CustomJobSettingsWidget * cjsw, const Job & job )
{
	foreach( JobService js, cjsw->getJobServices(job) ) {
		if( js.service().service().startsWith( "Fusion" ) )
			return js;
	}
	return JobService();
}

static void versionComboSetCurrent( RecordCombo * combo, CustomJobSettingsWidget * cjsw, JobList jobs )
{
	ServiceList sl;
	foreach( Job j, jobs )
		sl += findFusionService(cjsw, j).service();
	sl = sl.unique();

	if( sl.size() != 1 )
		combo->setCurrentIndex( -1 );
	else
		combo->setCurrent( sl[0] );
}

static void versionComboApplyCurrent( RecordCombo * combo, CustomJobSettingsWidget * cjsw, JobList jobs )
{
	// Apply changes from proxied widgets and get modified record list
	Service newFusionService = combo->current();
	if( !newFusionService.isRecord() ) {
		LOG_1( "Unabled to find fusion service for version " + combo->currentText() );
		return;
	}

	JobHistoryList history;
	foreach( JobFusion jf, jobs ) {
		JobService jobService = findFusionService( cjsw, jf );
		if( jobService.service() == newFusionService ) continue;

		JobHistory jh;
		jh.setJob( jf );
		jh.setMessage( "Job Settings Changed: Fusion Version changed from " + jobService.service().service() + " to " + newFusionService.service() );
		jh.setUser( User::currentUser() );
		jh.setHost( Host::currentHost() );
		jh.setColumnLiteral( "created", "NOW()" );
		jh.setType( JobHistoryType::recordByName( "info" ) );
		history += jh;

		jobService.setService( newFusionService );
		jobService.setJob( jf );
		cjsw->applyJobServices( jf, jobService );
	}
	history.commit();
}

JobFusionSettingsWidget::JobFusionSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
{
	setupUi( this );

	mFusionVersionCombo->setColumn( "service" );
	mFusionVersionCombo->setItems( Service::select().filter( "service", QRegExp( "^Fusion\\d" ) ) );

	connect( mFusionVersionCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobFusionSettingsWidget::~JobFusionSettingsWidget()
{
}

QStringList JobFusionSettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobFusionSettingsWidget::jobTypes()
{
	return QStringList( "Fusion" );
}

void JobFusionSettingsWidget::resetSettings()
{
	bool empty = mSelectedJobs.isEmpty();
	
	versionComboSetCurrent( mFusionVersionCombo, this, mSelectedJobs );

	setEnabled( !empty );

	CustomJobSettingsWidget::resetSettings();
}

void JobFusionSettingsWidget::applySettings()
{
	if(
		QMessageBox::warning( this, "Warning: This will reset the job",
									"Applying the settings will reset the job.",
		 							QMessageBox::Ok, QMessageBox::Cancel, QMessageBox::NoButton)
		!= QMessageBox::Ok ){
		return;
	}

	versionComboApplyCurrent( mFusionVersionCombo, this, mSelectedJobs );

	if( mMode == JobSettingsWidget::ModifyJobs )
		Job::updateJobStatuses( mSelectedJobs, "ready", true, true );

	CustomJobSettingsWidget::applySettings();
}

JobFusionVideoMakerSettingsWidget::JobFusionVideoMakerSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
{
	setupUi( this );

	mFusionVersionCombo->setColumn( "service" );
	mFusionVersionCombo->setItems( Service::select().filter( "service", QRegExp( "^Fusion\\d" ) ) );

	connect( mFusionVersionCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );

	connect( mFormatCodecCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );
	connect( mFormatCodecCombo, SIGNAL( activated(int) ), SLOT( updateOutputPath() ) );

	connect( mOutputPathEdit, SIGNAL( textChanged(const QString &) ), SLOT( settingsChange() ) );

	foreach( QString format, JobFusionVideoMaker::outputFormats() )
		foreach( QString codec, JobFusionVideoMaker::outputCodecs(format) )
			mFormatCodecCombo->addItem( format + " - " + codec );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobFusionVideoMakerSettingsWidget::~JobFusionVideoMakerSettingsWidget()
{
}

QStringList JobFusionVideoMakerSettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobFusionVideoMakerSettingsWidget::jobTypes()
{
	return QStringList( "FusionVideoMaker" );
}

void JobFusionVideoMakerSettingsWidget::resetSettings()
{
	bool empty = mSelectedJobs.isEmpty();
	
	versionComboSetCurrent( mFusionVersionCombo, this, mSelectedJobs );

	updateOutputPath();
	bool single = mSelectedJobs.size() == 1;
	mFormatCodecCombo->setEnabled( single );
	if( single ) {
		JobFusionVideoMaker j = mSelectedJobs[0];
		QString formatCodec = j.format() + " - " + j.codec();
		mFormatCodecCombo->setCurrentIndex( qMax(0,mFormatCodecCombo->findText( formatCodec )) );
	} else
		mFormatCodecCombo->setCurrentIndex( 0 );

	setEnabled( !empty );

	CustomJobSettingsWidget::resetSettings();
}

void JobFusionVideoMakerSettingsWidget::updateOutputPath()
{
	bool single = mSelectedJobs.size() == 1;
	mOutputPathEdit->setEnabled( single );
	if( single ) {
		JobFusionVideoMaker j = mSelectedJobs[0];
		QString format = mFormatCodecCombo->currentText().section(" - ",0,0);
		LOG_5( format );
		mOutputPathEdit->setText( format.isEmpty() ? j.outputPath() : JobFusionVideoMaker::updatePathToFormat(j.outputPath(),format) );
	} else
		mOutputPathEdit->clear();
}

void JobFusionVideoMakerSettingsWidget::applySettings()
{
	if(
		QMessageBox::warning( this, "Warning: This will reset the job",
									"Applying the settings will reset the job.",
		 							QMessageBox::Ok, QMessageBox::Cancel, QMessageBox::NoButton)
		!= QMessageBox::Ok ){
		return;
	}

	versionComboApplyCurrent( mFusionVersionCombo, this, mSelectedJobs );

	QStringList formatCodecParts = mFormatCodecCombo->currentText().split(" - ");
	if( formatCodecParts.size() == 2 ) {
		QString format = formatCodecParts[0], codec = formatCodecParts[1];
		foreach( JobFusionVideoMaker j, mSelectedJobs ) {
			QString outputPath = j.outputPath();
			QFileInfo opfi(outputPath);
			if( mOutputPathEdit->isEnabled() )
				j.setOutputPath( mOutputPathEdit->text() );
			else
				j.setOutputPath( JobFusionVideoMaker::updatePathToFormat(j.outputPath(),format) );
			j.setCodec( codec );
			mSelectedJobs.update(j);
		}
	}

	if( mMode == JobSettingsWidget::ModifyJobs ) {	
		mSelectedJobs.commit();
	
		Job::updateJobStatuses( mSelectedJobs, "ready", true, true );
	}

	CustomJobSettingsWidget::applySettings();
}

