
/*
 *
 * Copyright 2009 Blur Studio Inc.
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

/* $Author:
 * $LastChangedDate:
 * $Rev:
 * $HeadURL:
 */

#include <qmessagebox.h>

#include "recordproxy.h"

#include "jobtask.h"
#include "job.h"
#include "host.h"
#include "user.h"

#include "jobrealflowsettingswidget.h"

JobRealFlowSettingsWidget::JobRealFlowSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode )
: CustomJobSettingsWidget( parent, mode )
{
	JobRealFlowSettingsWidget::setupUi( this );

	mProxy = new RecordProxy( this );

	if( mode == JobSettingsWidget::ModifyJobs )
		mVerticalLayout->addLayout( mApplyResetLayout );
}

JobRealFlowSettingsWidget::~JobRealFlowSettingsWidget()
{
}

QStringList JobRealFlowSettingsWidget::supportedJobTypes()
{
	return jobTypes();
}

QStringList JobRealFlowSettingsWidget::jobTypes()
{
	return QStringList("RealFlow");
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

void JobRealFlowSettingsWidget::resetSettings()
{
	mProxy->setRecordList( mSelectedJobs, false, false );
	setEnabled( !mSelectedJobs.isEmpty() );

	CustomJobSettingsWidget::resetSettings();
}

void JobRealFlowSettingsWidget::applySettings()
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

