
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
 * $Id: remotetailwindow.cpp 6351 2008-04-17 21:07:37Z newellm $
 */

#include "remotetailwindow.h"
#include "remotelogserver.h"

RemoteTailWindow::RemoteTailWindow( QWidget * parent )
: QMainWindow( parent )
{
	setupUi(this);
	setAttribute( Qt::WA_DeleteOnClose, true );
	setWindowTitle( "Tailing Service Log..." );
	connect( mTailWidget->logConnection(), SIGNAL( remoteHostChanged() ), SLOT( remoteHostChanged() ) );
}

RemoteTailWindow::~RemoteTailWindow()
{
}

void RemoteTailWindow::remoteHostChanged()
{
	QString remoteHost = mTailWidget->logConnection()->remoteHostString();
	setWindowTitle( "Tailing service " + mServiceName + " of host " + remoteHost );
}

void RemoteTailWindow::setServiceName( const QString & serviceName )
{
	mServiceName = serviceName;
}

