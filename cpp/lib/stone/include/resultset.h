
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

#ifndef RESULT_SET_H
#define RESULT_SET_H

#include <qlist.h>

#include "blurqt.h"
#include "recordlist.h"

namespace Stone
{

class ResultSetIter;

///
/// A ResultSet holds a table of records
/// Each row consists of a set of records, such as multiple Records
/// returned from each row of a join query
///
/// Because the only data this class holds is a QList, which is implicitly shared,
/// it is very lightweight to copy.
class STONE_EXPORT ResultSet
{
public:
	ResultSet(const QList<RecordList> & results = QList<RecordList>()) : mData(results) {}

	ResultSetIter begin() const;
	ResultSetIter end() const;

	/// Returns the number of rows
	int rows() const { return mData.size() ? mData[0].size() : 0; }
	/// Returns the number of records wide this result set is
	/// This is unrelated to the number of columns in each record
	int columns() const { return mData.size(); }

	/// Returns the RecordList at column
	/// column should be in the range of 0 to columns()-1
	/// Column refers to a full record, not the individual columns in a record
	RecordList column(int column) const { return (column >= 0 && column < columns()) ? mData[column] : RecordList(); }

	/// Returns a list of the records in row
	RecordList row(int row) const;

	Record at(int row, int column) const;
	
	QList<RecordList> data() const { return mData; }
	
	ResultSet & append( ResultSet );
protected:
	QList<RecordList> mData;
};

class STONE_EXPORT ResultSetIter
{
public:
	ResultSet resultSet() const { return mResultSet; }
	RecordList operator*() const { return mResultSet.row(mRow); }
	RecordList row() const { return mResultSet.row(mRow); }
	
	/// Returns the record at column for the row this iter points to
	Record column(int column) { return mResultSet.at(mRow,column); }
	
	bool isValid() const { return mRow >= 0 && mRow < mResultSet.rows(); }
	
	ResultSetIter operator++() const { return ResultSetIter(mResultSet,mRow+1); }
	ResultSetIter operator--() const { return mRow == 0 ? mResultSet.end() : ResultSetIter(mResultSet,mRow-1); }
	ResultSetIter next() const { return ResultSetIter(mResultSet,mRow+1); }
	ResultSetIter previous() const { return mRow == 0 ? mResultSet.end() : ResultSetIter(mResultSet,mRow-1); }

protected:
	ResultSet mResultSet;
	int mRow;
	friend class ResultSet;
	ResultSetIter(const ResultSet & rs, int row) : mResultSet(rs), mRow(row) {}
};

inline ResultSetIter ResultSet::begin() const
{ return ResultSetIter( *this, 0 ); }

inline ResultSetIter ResultSet::end() const
{ return ResultSetIter( *this, rows() ); }

}; // namespace Stone

using Stone::ResultSet;
using Stone::ResultSetIter;

#endif // RESULT_SET_H