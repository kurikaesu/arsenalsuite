/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMIT_CODE

#include "gridtemplateitem.h"
#include "checklistitem.h"
#include "checkliststatus.h"
#include "elementstatus.h"
#include "task.h"
#include "assettype.h"
#include "versionfiletracker.h"

CheckListItemList GridTemplateItem::checkListItems() const
{
	CheckListItemList ret;
	QStringList sl = checkListItemsString().split(',');
	for( QStringList::Iterator it = sl.begin(); it != sl.end(); ++it )
		ret += CheckListItem( (*it).toUInt() );
	return ret;
}

void GridTemplateItem::setCheckListItems( CheckListItemList cl )
{
	QStringList sl;
	foreach( CheckListItem cti, cl )
	{
		uint itemKey = cti.key();
		if( itemKey )
			sl += QString::number(itemKey);
	}
	setCheckListItemsString( sl.join(",") );
}


QString GridTemplateItem::textFromElement( const Element & element ) const
{
	QStringList ret;
	ElementList el = element.children( Task::type() );
	foreach( Element e, el )
		if( e.assetType() == assetType() )
		{
			uint ct = columnType();
			if( ct & Status )
				ret += e.elementStatus().name();
			if( ct & AssignedUsers )
				ret += e.userStringList().join(",");
			if( ct & CheckListSummary )
				ret += CheckListItem::summary( e );
			if( ct & FileName ) {
				VersionFileTrackerList vft( e.trackers() );
				//st_foreach( VersionFileTrackerIter, it, vft )
					//ret += (*it).filePath();
			} if ( ct & DaysBid )
				ret += "";//QString::number( e.calcDaysBid() );
			if ( ct & DaysEstimated )
				ret += "";//QString::number( e.calcDaysEstimated() );
			if ( ct & DaysScheduled )
				ret += "";//QString::number( e.calcDaysScheduled() );
			if ( ct & DaysSpentActual )
				ret += "";//QString::number( e.calcDaysSpent() );
			if ( ct & DaysSpent8Hour )
				ret += "";//QString::number( e.calcDaysSpent( true ) );
			if ( ct & Dependencies ) {
				ElementList deps = element.dependencies();
				if( deps.isEmpty() )
					deps = element.dependants();
				QStringList dnl;
				foreach( Element e2, deps )
					dnl += e2.displayName();
				ret += dnl.join("\n");
			}
			CheckList clist = checkListItems();
			foreach( CheckListItem cli, clist )
				ret += cli.statusString( e );
		}
	return ret.join("\n");
}
/*
QColor GridTemplateItem::colorFromElement( const Element & element ) const
{
	ElementList el = element.children( Task::type() );
	foreach( Element e, el )
		if( e.assetType() == assetType() )
		{
			Task t(e);
			uint ct = columnType();
			if( ct & Status )
				return t.statusColor();
//			if( ct & AssignedUsers )
//			if( ct & CheckListSummary )
//			if( ct & FileName )
//			if( ct & DaysBid )
//			if( ct & DaysEstimated )
//			if( ct & DaysScheduled )
//			if( ct & DaysSpentActual )
//			if( ct & DaysSpent8Hour )
//			if( ct & Dependencies )
		}
	return Qt::white;
}
*/

#endif

