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
 * $LastChangedDate: 2008-02-04 11:45:25 -0800 (Mon, 04 Feb 2008) $
 * $Rev: 5578 $
 * $HeadURL: svn://newellm@ocelot/blur/trunk/cpp/lib/assfreezer/src/jobxsisettingswidget.cpp $
 */

#include <qmessagebox.h>

#include "recordproxy.h"

#include "jobxsi.h"
#include "jobtask.h"
#include "job.h"

#include "jobxsisettingswidget.h"

JobXSISettingsWidget::JobXSISettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
{
	JobXSISettingsWidget::setupUi( this );

	mProxy = new RecordProxy( this );

	connect( mRendererCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );
	connect( mWidthSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mHeightSpin, SIGNAL( valueChanged(int,bool) ), SLOT( settingsChange() ) );
	connect( mPassEdit, SIGNAL( textChanged(const QString&) ), SLOT( settingsChange() ) );
	connect( mMotionBlurCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );
	connect( mDeformMotionBlurCheck, SIGNAL( toggled(bool) ), SLOT( settingsChange() ) );

	mHeightSpin->setProxy( mProxy );
	mWidthSpin->setProxy( mProxy );
	mPassEdit->setProxy( mProxy );
	mMotionBlurCheck->setProxy( mProxy );
	mDeformMotionBlurCheck->setProxy( mProxy );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobXSISettingsWidget::~JobXSISettingsWidget()
{
}

QStringList JobXSISettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobXSISettingsWidget::jobTypes()
{
	return QStringList("XSI");
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

void JobXSISettingsWidget::resetSettings()
{
	mProxy->setRecordList( mSelectedJobs, false, false );
	bool empty = mSelectedJobs.isEmpty();
	setEnabled( !empty );

	JobXSIList xsiJobs( mSelectedJobs );
	bool rendererUnique = unique( xsiJobs.renderers() ).size() == 1;
	if( !empty && rendererUnique ) {
		int ri = 0;
		QStringList renderers = QStringList() << "trace" << "default" << "ogl" << "rapid";
		ri = renderers.indexOf( xsiJobs[0].renderer() );
		mRendererCombo->setCurrentIndex( ri >= 0 ? ri : 0 );
	} else
		mRendererCombo->setCurrentIndex( -1 );

	CustomJobSettingsWidget::resetSettings();
}

static const char * renderers [] = {"trace","default","ogl","rapid"};

void JobXSISettingsWidget::applySettings()
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
	JobXSIList xsiJobs( mSelectedJobs );
	
	int rci = mRendererCombo->currentIndex();
	if( rci >= 0 && rci < 4 )
		xsiJobs.setRenderers( renderers[rci] );

	if( mMode == JobSettingsWidget::ModifyJobs ) {
		xsiJobs.commit();

		Job::updateJobStatuses( mSelectedJobs, "ready", true );
	}

	CustomJobSettingsWidget::applySettings();
}

