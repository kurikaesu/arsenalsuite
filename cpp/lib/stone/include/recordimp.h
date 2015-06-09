
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

#ifndef RECORD_BASE_H
#define RECORD_BASE_H

#include <qatomic.h>
#include <qstring.h>
#include <qvector.h>

#include "blurqt.h"
#include "changeset.h"

class QVariant;
class QSqlQuery;

typedef QVector<QVariant> VariantVector;
template <class T> class QList;
class PGConnection;

namespace Stone {

class Record;
class KeyIndex;
class Field;
typedef QList<Field *> FieldList;
class Table;
class Connection;
class ChildIter;

	/**
 *  This class stores data for a single record in the layout
 *  determined by a \ref Table.  This class is wrapped by
 *  \ref Record, and is only used internally.
 *  \ingroup Stone
 */
class STONE_EXPORT RecordImp
{
public:
	// Loads mValues with the values in toLoad.  There needs to be table->fields().size() values in the array
	// If toLoad=0 then constructs a new record with empty values. If table is specified
	// then each value is filled with the corrosponding field->defaultValue()
	RecordImp( Table * table, QVariant * toLoad = 0 );
	
	// Loads mValues with the data in the sql query, starting with queryPosOffset.
	// If fields is 0, then q should have data for every non-local field that doesn't have NoDefaultSelect set
	// otherwise fields should contain the full list of fields contained in q
	RecordImp( Table * table, QSqlQuery & q, int queryPosOffset = 0, FieldList * fields = 0 );
	
	// Loads mValues with the data in the sql query, indexed by the entries in the queryColPos array
	// If fields is 0, then q should have data for every non-local field that doesn't have NoDefaultSelect set
	// otherwise fields should contain the full list of fields contained in q
	RecordImp( Table * table, QSqlQuery & q, int * queryColPos, FieldList * fields = 0 );

	// Loads mValues with the data in the sql query, indexed by the entries in the queryColPos array
	// queryColPos.size() should equal the number of non-local columns in the table
	// If the column in queryColPos is -1, then the column was not selected and the not selected bit should
	// be set
	RecordImp( Table * table, QSqlQuery & q, const QVector<int> & queryColPos );

	~RecordImp();
	
	/// Adds 1 to the reference count
	void ref();

	/// Subtracts 1 from the reference count
	/// delete this if the reference count falls to 0
	void deref();

	/// Returns the current reference count.
	int refCount() const { return mRefCount; }

	///  Fills the array of QVariants pointed to by \param v
	///  with the values in this record.  \param v must
	///  point to an array with as many values as this
	///  RecordImp's table has columns.
	void get( QVariant * v );

	/// Fills the QVariant array pointed to by \param v with the values
	/// in this object.     \param v must
	///  point to an array with as many values as this
	///  RecordImp's table has columns.
	void set( QVariant * v );

	void updateChildren();
	
	///  Returns the QVariant value at the position \param col
	const QVariant & getColumn( int col ) const;
	
	const QVariant & getColumn( Field * f ) const;
	
	/// Sets the QVariant value at the position \param col
	RecordImp * setColumn( int col, const QVariant & v );
	RecordImp * setColumn( Field * f, const QVariant & v );
	
	/// Sets the QVariant value at position \param col
	/// This function does not modify mState, it does clear mNotSelectedBits
	void fillColumn( int col, const QVariant & v );
	
	/// Returns the QVariant value at \param column
	const QVariant & getValue( const QString & column ) const;

	/// Sets the QVariant value at \param column
	/// May return a newly allocated and referenced RecordImp, if so
	/// will dereference this
	RecordImp * setValue( const QString & column, const QVariant & v );

	/// Returns the table this recordimp belongs to
	Table * table() const { return mTable; }

	/// Creates a copy of this RecordImp
	RecordImp * copy( bool attachToPristine = true );

	/// Returns the primary key for this record
	/// This is inlined as a special case because we can ensure the primary key
	/// is always selected(doesn't have NoDefaultSelect set).
	uint key() const;
	
	/// Commits this record to the database.
	/// If \param newPrimaryKey is true, a new primary key
	/// is generated, else the existing key is used.
	/// If sync is false, the sql will be executed in the
	/// background thread.
	/// May return a different referenced RecordImp, if so
	/// will dereference this
	RecordImp * commit();

	/// Removes the record from the database.
	void remove();

	void setColumnModified( uint col, bool modified );
	bool isColumnModified( uint col ) const;
	void clearModifiedBits();
	
	bool isColumnSelected( uint col );
	FieldList notSelectedColumns();
	
	RecordImp * setColumnLiteral( uint col, bool modified );
	bool isColumnLiteral( uint col ) const;
	void clearColumnLiterals();
	
	enum State {
		COMMITTED = 1,
		MODIFIED = 2,
		DELETED = 4,
		// This is to prevent allocations for "empty" records
		// Each table instance will have it's own empty recordimp
		// essentially it would be like Record keeping mImp = 0, except
		// that you still have a pointer to the table.
		EMPTY_SHARED = 8,
		// This is used (primarily by undo manager) to commit all fields
		// without having to mark each as modified.
		COMMIT_ALL_FIELDS = 16,
		// This is used when a RecordImp is thrown away either because the
		// data was committed or because the changeset it belonged to was
		// rolled back.
		DISCARDED = 32,
		INSERT_PENDING = 64,
		UPDATE_PENDING = 128,
		DELETE_PENDING = 256,
		INSERT_PENDING_CHILD = 512,
		MODIFIED_SINCE_QUEUED = 1024,
		HOLDS_OLD_VALUES = 2048,
	};

	QString debugString();

	// Used by Table to mark the record committed.  Pass 0 to clear the COMMITTED
	// flag.
	void setCommitted(uint key);
	
	int mState;

	VariantVector * values() { return mValues; }

	static int totalCount();
	
	bool hasVersions() const { return mNext != 0; }

	RecordImp * first() {
		RecordImp * ret = this;
		while( ret->mParent )
			ret = ret->mParent;
		return ret;
	}
	
	bool isChild( RecordImp * imp ) {
		while( imp ) {
			if( imp->mParent == this ) return true;
			imp = imp->mParent;
		}
		return false;
	}

	bool hasChild( RecordImp * imp ) {
		// Children always come after their parent in the list
		// There should never be a circumstance where a non-child
		// gets in between
		return imp && imp->mNext && imp->mNext->mParent == imp;
	}
protected:
	RecordImp * version(const ChangeSet & cs);
	QAtomicInt mRefCount;
	Table * mTable;
	VariantVector * mValues;
	char * mModifiedBits;
	char * mLiterals;
	char * mNotSelectedBits;
	// Circular linked list of RecordImps representing this record
	// Each entry in the list is either the pristine copy, or a modified
	// copy, possibly associated with a changeset.  The circular nature 
	// avoids the needs for a double linked list, and is plenty fast since
	// there should rarely be more than two entries, the pristine and the modified.
	RecordImp * mNext;
	RecordImp * mParent;
	ChangeSetWeakRef mChangeSet;
	friend class Record;
	friend class RecordList;
	friend class Table;
	friend class KeyIndex;
	friend class Connection;
	friend class ::PGConnection;
	friend class ChildIter;
};

} //namespace

using Stone::RecordImp;

#endif // RECORD_BASE_H

