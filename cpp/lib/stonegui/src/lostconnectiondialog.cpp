
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: lostconnectiondialog.cpp 9918 2010-05-26 21:52:41Z newellm $
 */

#include <stdlib.h>

#include <qapplication.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <qeventloop.h>

#include "configdbdialog.h"
#include "connection.h"
#include "lostconnectiondialog.h"
#include "freezercore.h"
#include "process.h"

LostConnectionDialog::LostConnectionDialog( Stone::Connection * c, const QString & errorMsg )
: QDialog( 0 )
, mConnection( c )
, mSecondsToRetry( 15 )
{
	mUI.setupUi( this );
	connect( mUI.mExitButton, SIGNAL( clicked() ), SLOT( quit() ) );
	connect( mUI.mRetryButton, SIGNAL( clicked() ), SLOT( retryNow() ) );
	connect( mUI.mSettingsButton, SIGNAL( clicked() ), SLOT( showSettings() ) );
	mUI.mSecondsLabel->setText( "15" );
	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), SLOT( retryConnection() ) );
	mTimer->start( 1000 );

	mUI.mErrorLabel->setText( errorMsg );
	setFixedSize( size() );
}

void LostConnectionDialog::retryNow()
{
	mSecondsToRetry = 0;
	retryConnection();
}

void LostConnectionDialog::quit()
{
	if( qApp->applicationName() == "Assburner" )
		killAll( "abpsmon.exe" );
	QDialog::reject();
	QApplication::exit( 1 );
	exit( 0 );
}

void LostConnectionDialog::retryConnection()
{
	mSecondsToRetry--;
	if( mSecondsToRetry <= 0 ) {
		if( mConnection->reconnect() ) {
			accept();
			return;
		} else {
			mUI.mErrorLabel->setText( mConnection->lastErrorText() );
		}
		mSecondsToRetry = 15;
	}
	mUI.mSecondsLabel->setText( QString::number( mSecondsToRetry ) );
}

void LostConnectionDialog::reject()
{
	return;
}

void LostConnectionDialog::showSettings()
{
	mTimer->stop();
	ConfigDBDialog* cdb = new ConfigDBDialog( this );
	bool retry = cdb->exec() == QDialog::Accepted;
	delete cdb;
	mSecondsToRetry = 15;
	mTimer->start( 1000 );
	if( retry )
		retryNow();
}

