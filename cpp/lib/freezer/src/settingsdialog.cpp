
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
 * $LastChangedDate: 2011-02-14 13:26:36 -0800 (Mon, 14 Feb 2011) $
 * $Rev: 10941 $
 * $HeadURL: svn://newellm@ocelot/blur/trunk/cpp/lib/assfreezer/src/settingsdialog.cpp $
 */

#include <qpushbutton.h>
#include <qlineedit.h>
#include <qfiledialog.h>
#include <qspinbox.h>

#include "settingsdialog.h"

extern Options options;

SettingsDialog::SettingsDialog( QWidget * parent )
: QDialog( parent )
, mChanges( false )
{
	setupUi( this );

	connect( ApplyButton, SIGNAL( clicked() ), SLOT( slotApply() ) );
	connect( OKButton, SIGNAL( clicked() ), SLOT( slotApply() ) );
	opts = options;

	mFrameCyclerPath->setText( opts.frameCyclerPath );
	mFrameCyclerArgs->setText( opts.frameCyclerArgs );
	mJobLimitSpin->setValue( opts.mLimit );
	mDaysLimitSpin->setValue( opts.mDaysLimit );
	mRefreshIntervalSpin->setValue( opts.mRefreshInterval );
	mAutoRefreshOnWindowActivationCheck->setChecked( opts.mAutoRefreshOnWindowActivation );
	mRefreshOnViewChangeCheck->setChecked( opts.mRefreshOnViewChange );
	mDragStartDistanceSpin->setValue( qApp->startDragDistance() );
	
	connect( mFrameCyclerPath, SIGNAL( textChanged( const QString & ) ), SLOT( changes() ) );
	connect( mFrameCyclerArgs, SIGNAL( textChanged( const QString & ) ), SLOT( changes() ) );
	connect( mFrameCyclerPathButton, SIGNAL( clicked() ), SLOT( selectFrameCyclerPath() ) );
	connect( mJobLimitSpin, SIGNAL( valueChanged( int ) ), SLOT( changes() ) );
	connect( mDaysLimitSpin, SIGNAL( valueChanged( int ) ), SLOT( changes() ) );
	connect( mRefreshIntervalSpin, SIGNAL( valueChanged( int ) ), SLOT( changes() ) );
	connect( mAutoRefreshOnWindowActivationCheck, SIGNAL( toggled(bool) ), SLOT( changes() ) );
	connect( mRefreshOnViewChangeCheck, SIGNAL( toggled(bool) ), SLOT( changes() ) );
	connect( mDragStartDistanceSpin, SIGNAL( valueChanged(int) ), SLOT( changes() ) );

	ApplyButton->setEnabled( false );
}

void SettingsDialog::slotApply()
{
	if( mChanges ){
		opts.frameCyclerPath = mFrameCyclerPath->text();
		opts.frameCyclerArgs = mFrameCyclerArgs->text();
		opts.mLimit = mJobLimitSpin->value();
		opts.mDaysLimit = mDaysLimitSpin->value();
		opts.mRefreshInterval = mRefreshIntervalSpin->value();
		opts.mAutoRefreshOnWindowActivation = mAutoRefreshOnWindowActivationCheck->isChecked();
		opts.mRefreshOnViewChange = mRefreshOnViewChangeCheck->isChecked();
		qApp->setStartDragDistance( mDragStartDistanceSpin->value() );
		options = opts;
		mChanges = false;
		ApplyButton->setEnabled( false );
		emit apply();
	}
}

void SettingsDialog::changes()
{
	mChanges = true;
	ApplyButton->setEnabled( true );
}

void SettingsDialog::selectFrameCyclerPath()
{
	QString fcp = QFileDialog::getOpenFileName( this,
		"Choose FrameCycler Executable", "C:\\Program Files\\", "Executables (*.exe)" );

	if( QFile::exists( fcp ) ){
		opts.frameCyclerPath = fcp;
		mFrameCyclerPath->setText( fcp );
	}
}

