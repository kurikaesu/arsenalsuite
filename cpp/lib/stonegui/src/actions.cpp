
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
 * $Id: actions.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qaction.h>
#include <qicon.h>

#include "actions.h"

QAction * separatorAction( QObject * parent )
{
	QAction * action = new QAction( parent );
	action->setSeparator( true );
	return action;
}

QAction * checkableAction( const QString & text, QObject * parent, bool checked, QObject * recv, const char * member, const QIcon & icon )
{
	QAction * action = new QAction( text, parent );
	action->setCheckable( true );
	action->setChecked( checked );
	if( recv && member )
		recv->connect( action, SIGNAL( triggered(bool) ), member );
	if( !icon.isNull() )
		action->setIcon( icon );
	return action;
}

QAction * connectedAction( const QString & text, QObject * parent, QObject * recv, const char * member, const QIcon & icon )
{
	QAction * action = new QAction( text, parent );
	if( recv && member )
		recv->connect( action, SIGNAL( triggered(bool) ), member );
	if( !icon.isNull() )
		action->setIcon( icon );
	return action;
}

