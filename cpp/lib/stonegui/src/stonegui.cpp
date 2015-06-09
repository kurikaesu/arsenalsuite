
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
 * $Id: stonegui.cpp 13595 2012-09-17 21:35:20Z newellm $
 */

#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qtimer.h>
#include <qthread.h>

#include "stonegui.h"
#include "database.h"
#include "connection.h"
#include "lostconnectiondialog.h"
#include "imagesequenceprovider.h"

static ConnectionWatcher * sWatcher = 0;

ConnectionWatcher::ConnectionWatcher()
: mDialog( 0 )
, mMutex(QMutex::Recursive)
{
	Connection * c = Database::current()->connection();
	connect( c, SIGNAL( connectionLost2() ), SLOT( connectionLost() ) );
}

void ConnectionWatcher::connectionLost()
{
	LOG_5( "Here1" );
	mMutex.lock();
	LOG_5( "Here2" );
	if( QThread::currentThread() != QApplication::instance()->thread() )
		QMetaObject::invokeMethod( this, "showDialog", Qt::BlockingQueuedConnection );
	else
		showDialog();
	mMutex.unlock();
}

void ConnectionWatcher::showDialog()
{
	LOG_5( "Here3" );
	if( mDialog == 0 ) {
		Connection * c = Database::current()->connection();
		mDialog = new LostConnectionDialog( c, c->lastErrorText() );
		mDialog->exec();
		delete mDialog;
		mDialog = 0;
	}
}

void ConnectionWatcher::connected()
{
}

ConnectionWatcher * ConnectionWatcher::connectionWatcher()
{
	if( !sWatcher )
		sWatcher = new ConnectionWatcher();
	return sWatcher;
}


void initStoneGui()
{
	ConnectionWatcher::connectionWatcher();
	registerBuiltinImageSequenceProviderPlugins();
}

