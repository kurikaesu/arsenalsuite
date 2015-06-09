
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
 * $Id: recentvaluesui.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qtimer.h>

#include "recentvaluesui.h"

RecentValuesMenu::RecentValuesMenu( const QString & title, const RecentValues & recentValues, QWidget * parent )
: QMenu( title, parent )
, mRecentValues( recentValues )
{
	connect( this, SIGNAL( triggered( QAction * ) ), SLOT( childActionTriggered( QAction * ) ) );
	refresh();
}
	
void RecentValuesMenu::childActionTriggered( QAction * action )
{
	emit recentValueTriggered( action->data(), action->text() );
}

void RecentValuesMenu::refresh()
{
	clear();
	foreach( RecentValues::RecentNameValue rnv, mRecentValues.recentNameValues() ) {
		QAction * act = addAction( rnv.name );
		act->setData( rnv.value );
	}
}

void RecentValuesMenu::addRecentValue( const QVariant & value, const QString & name )
{
	mRecentValues.addRecentValue( value, name );
	QTimer::singleShot( 10, this, SLOT( refresh() ) );
}

