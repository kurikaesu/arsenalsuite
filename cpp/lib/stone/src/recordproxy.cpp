
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
 * $Id$
 */

#include "recordproxy.h"

namespace Stone {

RecordProxy::RecordProxy( QObject * parent )
: QObject( parent )
{
}

RecordList & RecordProxy::records()
{
	return mRecords;
}

void RecordProxy::applyChanges( bool commitChanges )
{
	emit updateRecordList();
	if( commitChanges )
		mRecords.commit();
}

void RecordProxy::setRecordList( const RecordList & records, bool apply, bool commitChanges )
{
	if( apply )
		applyChanges( commitChanges );
	mRecords = records;
	recordListChange();
}

void RecordProxy::clear()
{
	mRecords.clear();
	emit recordListChange();
}

} // namespace
