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
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/jobshakesettingswidget.cpp $
 */

#include <qmessagebox.h>

#include "recordproxy.h"

#include "jobtask.h"
#include "job.h"

#include "jobshakesettingswidget.h"

JobShakeSettingsWidget::JobShakeSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
{
	JobShakeSettingsWidget::setupUi( this );

	mProxy = new RecordProxy( this );

	/* Restart Settings connections */
	connect( CodecCombo, SIGNAL( activated(int) ), SLOT( settingsChange() ) );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobShakeSettingsWidget::~JobShakeSettingsWidget()
{
}

QStringList JobShakeSettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobShakeSettingsWidget::jobTypes()
{
	return QStringList("Shake");
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

void JobShakeSettingsWidget::resetSettings()
{
	mProxy->setRecordList( mSelectedJobs, false, false );
	bool empty = mSelectedJobs.isEmpty();
	setEnabled( !empty );

	if( !empty ) {
		Job js = mSelectedJobs[0];
		CodecCombo->setEditText( js.getValue( "codec" ).toString() );
	}
}

void JobShakeSettingsWidget::applySettings()
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
		mSelectedJobs.commit();

		Job::updateJobStatuses( mSelectedJobs, "new", true );
	}
}

