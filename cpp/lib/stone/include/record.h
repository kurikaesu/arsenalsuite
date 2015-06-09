
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

#ifndef RECORD_H
#define RECORD_H

#include "blurqt.h"

#include <qstring.h>
#include <qvariant.h>

template<class T> class QList;

namespace Stone {
class ChangeSet;
class Field;
class RecordImp;
class Table;
class TableSchema;

typedef QList<Field*> FieldList;

STONE_EXPORT bool isChanged( const QString &, const QString & );

/**
 * \class Record
 * 
 * Record is the base class that all user-defined tables use.
 *
 * A Record is essentially a database row, with accessors to
 * set/get any column data.
 *
 * \ingroup Stone
 * @{
 */
class STONE_EXPORT Record
{
public:

	mutable RecordImp * mImp;
	
	Record( RecordImp * imp, bool notused = false );

	Record( Table * table = 0 );

	Record( const Record & r );
	
	~Record();

	Record getVersion( const ChangeSet & cs ) const;
	// This is the same as calling getVersion with an invalid changeset
	// Provided for the sake of readability.
	Record pristine() const;
	// Returns the record that this record's changes are based off of
	// May be the pristine or another changeset
	Record parent() const;
	
	Record & operator=( const Record & r );

	bool operator==( const Record & other ) const;
	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	/** Returns true if this object is a valid
	 * object, ie it contains some data.
	 * It does not matter whether the object is
	 * committed, deleted, or a modifed new record
	 *
	 * Use this method in derived record classes
	 * to check for inheritance:
	 *
	 *  void isUserRecord( const Record & r )
	 * {
	 *     return User(r).isValid();
	 * }
	 * This is the same as
	 * return r.table() ? r.table()->inherits( User::table() ) : false;
	 *
	 * Host host = Host()
	 *  // host.isValid() == false
	 * host.setHostName( 'test' )
	 *  // host.isValid() == true
	 * User(host).isValid() == false
	 */
	bool isValid() const;

	/// Returns true if this record is in the database
	bool isRecord() const;

	ChangeSet changeSet() const;
	
	/// Returns this record's primary key
	uint key( bool generate = false ) const;
	/// Generates a primary key if needed, and returns it
	uint generateKey() const;
	
	/// Returns the value in column 'column'
	const QVariant & getValue( const QString & column ) const;

	/// Sets the value in 'column' to 'value'
	Record & setValue( const QString & column, const QVariant & value );

	Record & setForeignKey( const QString & column, const Record & other );
	Record & setForeignKey( int column, const Record & other );
	Record & setForeignKey( Field * field, const Record & other );
	
	Record foreignKey( const QString & column ) const;
	Record foreignKey( int column ) const;
	Record foreignKey( Field * field, int lookupMode = 0x3 /*Index::UseCache|Index::UseSelect*/ ) const;
	
	/// Sets the column to a literal SQL value that will be used for
	/// the next update/commit.  A null QString will clear the literal
	/// for this column.
	Record & setColumnLiteral( const QString & column, const QString & literal );
	
	/// Returns the literal value assigned to this column.
	/// Returns QString::null if there is none.
	QString columnLiteral( const QString & column ) const;
	
	/// Gets or sets the value in the column at position 'column'
	/// Column positions are not guaranteed to be in any
	/// particular order
	const QVariant & getValue( int column ) const;
	const QVariant & getValue( Field * field ) const;
	
	Record & setValue( int column, const QVariant & value );
	Record & setValue( Field * field, const QVariant & value );
	
	QString stateString() const;

	/// human readable output
	QString displayName() const;
	/// String representation for lower level logging
	QString debug() const;
	QString dump() const;
	QString changeString() const;

	// If refreshExisting is false, only fields in the list are selected that
	// have not previously been selected, otherwise all fields in the list are
	// selected.  If fields is empty and refreshExisting is false, all unselected
	// fields are selected.
	void selectFields( FieldList fields = FieldList(), bool refreshExisting = false );

	/// Removes this record from the database and indexes
	/// return value of -1 indicates error. 0 indicates that
	/// the record was not in the database, 1 indicates the
	/// record was removed
	int remove();

	/// Reloads this record from the database
	Record & reload( bool lockForUpdate = false );

	/// Commits this record to the database
	///
	/// If sync is false and there is a worker thread
	/// running, then this record will be committed by
	/// the worker thread, and this function will
	/// return before the primary key has been
	/// set and before and recordsAdded/Updated signals
	/// have been emmitted.
	
	/// Can throw SqlException and LostConnectionException.
	Record & commit();

	RecordImp * imp() const { return mImp; }
	
	bool isUpdated( Field * f = 0 ) const;

	Table * table() const;

	/// If destTable is null then the copy is made from the current record's table
	/// If destTable has a different TableSchema than the current record, then each
	/// field is copied by database name, and if no match is found then by method name
	Record copy( Table * destTable = 0 ) const;
	
	/// Number of Record objects in existence
	static int totalRecordCount();
	/// Number of RecordImp objects in existence
	static int totalRecordImpCount();

	friend class Stone::RecordImp;
	friend class RecordList;
	RecordImp * current(bool read = true) const;
protected:
	void checkImpType(TableSchema * ts);
};

} // namespace

using Stone::Record;

#include "recordimp.h"

inline bool Record::isValid() const
{ return bool(mImp) && !(mImp->mState & (RecordImp::EMPTY_SHARED|RecordImp::DISCARDED)); }

inline Stone::Table * Record::table() const
{ return mImp ? mImp->table() : 0; }

inline uint Record::key( bool generate ) const
{ return generate ? generateKey() : (mImp ? mImp->key() : 0); }


Q_DECLARE_METATYPE(Record)

#include "recordlist.h"

#endif // RECORD_H

