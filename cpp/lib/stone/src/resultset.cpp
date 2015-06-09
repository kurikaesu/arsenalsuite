
/*
 *
 * Copyright 2012 Blur Studio Inc.
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

#include "resultset.h"

RecordList ResultSet::row(int row) const
{
	RecordList ret;
	for( int i = 0; i < columns(); i++ )
		ret += at(row,i);
	return ret;
}

Record ResultSet::at(int row, int column) const
{
	if( row < 0 || column < 0 || column >= mData.size() )
		return Record();
	const RecordList & rl = mData.at(column);
	if( row >= rl.size() )
		return Record();
	return rl[row];
}

ResultSet & ResultSet::append( ResultSet rs )
{
	if( !mData.isEmpty() && rs.columns() != columns() ) {
		LOG_1( "Attempt to append a resultset with different number of columns" );
		return *this;
	}
	
	if( mData.isEmpty() )
		*this = rs;
	else
		for( int i = 0; i < columns(); ++i )
			mData[i] += rs.column(i);
	return *this;
}
