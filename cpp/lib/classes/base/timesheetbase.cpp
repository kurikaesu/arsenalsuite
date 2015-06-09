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

#include "element.h"
#include "timesheet.h"

TimeSheetList TimeSheet::recordsByElementAndDate(const Element & element, QDateTime dateTime)
{
	TimeSheetList ret;
	QDate date = dateTime.date();
	TimeSheetList tsl = recordsByElement( element );
	foreach( TimeSheet ts, tsl )
		if( ts.dateTime().date() == date )
			ret += ts;
	return ret;
}

/*
static void updateAssetDates( TimeSheetList tsl )
{
	ElementList toCommit;
	foreach( TimeSheet ts, tsl ) {
		Element e = ts.element();
		QDate d = ts.dateTime().date();
		if( e.startDate() > d )
			e.setStartDate( d );
		if( e.dateComplete() < d )
			e.setDateComplete( d );
		toCommit += e;
	}
	toCommit.commit();
}

void TimeSheetSchema::postInsert( RecordList rl )
{
	updateAssetDates( rl );
	TableSchema::postInsert( rl );
}

void TimeSheetSchema::postUpdate( const Record & updated, const Record & r )
{
	updateAssetDates( updated );
	TableSchema::postUpdate( updated, r );
}
*/

#endif

