
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

#include "record.h"

#ifndef RECORD_LIST_H
#define RECORD_LIST_H

#include <qlist.h>

class QRegExp;

namespace Stone {

class Expression;
class Table;
class RecordImp;
class RecordIter;
class ImpList;

typedef QList<RecordImp*>::Iterator ImpIter;
typedef QList<RecordImp*>::ConstIterator ConstImpIter;
#define st_foreach(iterclass, iter,list) for( iterclass iter = list.begin(); iter != list.end(); ++iter )


/**
 *  \class RecordList
 *  \brief  This class encapsulates a list of Records.
 *  This class encapsulates a list of Records and can perform
 *  many operations on those records.
 *
 *  This class is implicitly shared, and is copy-on-write.
 *
 *  \ingroup Stone
 * @{
 */
class STONE_EXPORT RecordList
{
public:
	RecordList();
	RecordList( const RecordList & other, Table * table );
	RecordList( const RecordList & other );
	RecordList( const Record & rec, Table * table = 0 );
	
	~RecordList();

	RecordList & operator=( const RecordList & );

	/// Returns the \param nth \class Record in the list
	/// or an empty invalid \class Record if \param nth
	/// is out of range
	Record operator []( uint nth ) const;

	/// Appends \param record to the list
	RecordList & operator += ( const Record & record );

	/// Appends \ref RecordList \param list to this list
	RecordList & operator += ( const RecordList & list );

	/// Appends \ref RecordImp \param imp to this list
	RecordList & operator += ( RecordImp * imp);

	/// Returns a new \ref RecordList, with the contents of
	/// \param list appended to the contents of this.
	RecordList operator + ( const RecordList & list ) const;

	/// Removes all occurances of the contents of \param list
	/// from this list.
	/// Returns a reference to this.
	RecordList & operator -= ( const RecordList & list );
	RecordList & operator -= ( const Record & other );
	
	/// Returns a copy of this list with all occurances of
	/// \param list removed.
	RecordList operator - ( const RecordList & list ) const;
	
	/// Returns the intersection of the two lists
	RecordList operator & ( const RecordList & ) const;

	/// Sets this as the intersection between this and other
	/// returns a reference to this
	RecordList & operator &= ( const RecordList & other );
	
	/// Returns true if the intersection between this and other
	/// is not empty
	bool operator && ( const RecordList & other ) const;
	
	/// Returns the union of this and other
	RecordList operator | ( const RecordList & other ) const;
	
	/// Sets this to the union of this and other
	/// Returns a reference to this
	RecordList & operator |= ( const RecordList & other );
	
	/// Returns true if the union of this and other
	/// is not empty.
	bool operator || ( const RecordList & other ) const;

	/// Returns true if \param list contains the same
	/// records as this list in the same order.
	bool operator==( const RecordList & list ) const;

	/// Same as !(operator==(other))
	bool operator!=( const RecordList & list ) const { return !(this->operator==(list)); }

	/// Returns a RecordIter object positioned on this
	/// list at position \param n. If n is out of range,
	/// then the returned RecordIter will be the
	/// same as end().
	RecordIter at( uint n ) const;

	// slice( INT_MAX, INT_MIN, -1 ) returns the list backwards
	RecordList slice( int start, int end = INT_MAX, int step = 1 );
	
	void reverse();
	
	/// Returns the RecordImp * for the \param nth record
	/// in this list.  Returns 0 if \param nth is out of range.
	RecordImp * imp( uint nth ) const;

	/// Returns the index of \param record in this list.
	/// Returns -1 if \param record is not found in this list.
	int findIndex( const Record & record );

	/// Returns a \ref RecordIter object positioned on
	/// the first occurance of \param record in the list
	/// if record is not found in the list, the returned
	/// value will be end().
	RecordIter find( const Record & record );

	/// Appends \param record to this list.
	///
	/// \see RecordList::operator+=( const Record & )
	void append( const Record & record );
	
	/// Insert \param r beforer the position pointed
	/// to by \param it.  If it points to end, \param r
	/// will be appended to this list.
	void insert( RecordIter it, const Record & r );
	
	/// Follows python behavior, if pos < 0 then it counts from end of list backwards.
	/// There is no invalid pos, if pos > size then it appends the item, or if pos < -size
	/// then the item is prepended
	void insert( int pos, const Record & r );

	/// Finds the first occurance of a committed record and replaces it
	/// with 'record'.  Used to update the unmodified record when iterating
	/// through a list, because of copy-on-write semantics.
	bool update( const Record & );
	
	/// Removes the first occurance of \param record from this list.
	/// Will remove a different version of the same record
	int remove( const Record & record );
	
	/// Removes the first occurance of the record that's RecordImp
	/// is equal to \param imp.
	int remove( RecordImp * imp );

	/// Removes the record pointed to by \param iter, and returns
	/// a new RecordIter pointing to the next record in the list.
	RecordIter remove( const RecordIter & iter );

	/// Follows python behavior.  If i < 0 then the popped position is size() - i
	/// If index is out of range then an empty record is returned
	Record pop( int i = -1 );
	
	/// Removes all records from the list.
	void clear();

	/// Returns true if this list contains \param record
	/// Different versions of the same record are considered a match
	bool contains( const Record & record ) const;

	/// Returns true if this list contains \param imp
	/// Compares imps direct, will not return true if the list contains a different version of the record
	bool contains( RecordImp * imp ) const;

	/// Returns true if this list is empty.
	bool isEmpty() const;

	/// Returns the number of records in the list
	uint size() const;
	/// Returns the number of records in the list
	uint count() const;

	void selectFields( FieldList fields = FieldList(), bool refreshExisting = false );
	
	/// Calls Record::commit() on each record in this list.
	void commit();

	/// Calls Record::remove() on each record in this list.
	/// Returns -1 on error, else the number of records
	/// actually removed from the database.
	int remove();

	/// Returns a QList<QVariant> list of values, filled with
	/// the returned value after calling getValue( \param column )
	/// on each value in this list.
	QList<QVariant> getValue( const QString & column ) const;

	/// Calls Record::setValue( \param column, \param value ) on
	/// each record in this list.
	void setValue( const QString & column, const QVariant & value );

	/// Returns a QList<QVariant> list of values, filled with
	/// the returned value after calling getValue( \param column )
	/// on each value in this list.
	QList<QVariant> getValue( int column ) const;
	QList<QVariant> getValue( Field * f ) const;
	
	/// Calls Record::setValue( \param column, \param value ) on
	/// each record in this list.
	void setValue( int column, const QVariant & value );
	void setValue( Field * f, const QVariant & value );
	
	/// Returns the return values of Record::foreignKey for each record.
	RecordList foreignKey( int column, int lookupMode = 0x7 /*Index::UseCache | Index::UseSelect | Index::PartialSelect*/ ) const;
	RecordList foreignKey( const QString & column, int lookupMode = 0x7 /*Index::UseCache | Index::UseSelect | Index::PartialSelect*/ ) const;
	RecordList foreignKey( Field * f, int lookupMode = 0x7 /*Index::UseCache | Index::UseSelect | Index::PartialSelect*/ ) const;
	
	RecordList & setForeignKey( int column, const Record & fkey );
	RecordList & setForeignKey( const QString & column, const Record & fkey );
	RecordList & setForeignKey( Field * f, const Record & fkey );
	
	/// Calls Record::setColumnLiteral(\param column,\param value) on
	/// each record in this list.
	void setColumnLiteral( const QString & column, const QString & value );

	/// Returns a list of keys,
	/// If idx is -1 it returns the primary keys
	/// else it returns the column 
	/// with index idx. null/zero values are not
	/// returned.
	QList<uint> keys( int idx=-1 ) const;

	/// Returns a comma separated list of primary keys
	QString keyString() const;

	/// Returns a RecordIter object that points to the
	/// first record in this list.
	RecordIter begin() const;

	/// Returns a RecordIter object that points represents
	/// the end of this list.
	RecordIter end() const;

	/// Returns the table this list is filtering.
	/// This will return 0 for RecordList objects, but
	/// will return a valid table for other classes that
	/// inherit from RecordList(classes generated with classgen).
	Table * table() const;

	/// Returns a filtered list. Compared with QVariant::operator== (const QVariant &),
	/// If keepMatches is true than the returned list will be all the matches
	/// else it will be all the non-matches
	RecordList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;

	/// Returns a filtered list. Each QVariant value is compared by calling toString(), then
	/// matched with the regular expression \param re.
	/// If keepMatches is true than the returned list will be all the matches
	/// else it will be all the non-matches
	RecordList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;

	RecordList filter( const Expression &, bool keepMatches ) const;
	
	/// Returns a map, with the key equal to column for each value in the list
	QMap<QString,RecordList> groupedBy( const QString & column ) const;

	template<class KEY,class LIST> QMap<KEY,LIST> groupedBy( const QString & column ) const { return groupedBy<KEY,LIST,KEY,KEY>(column); }
	template<class KEY,class LIST,class VARIANT_TYPE,class KEY_CAST_TYPE> QMap<KEY,LIST> groupedBy( const QString & column ) const;
	template<class KEY> QMap<KEY,RecordList> groupedBy( const QString & column ) const { return groupedBy<KEY,RecordList>(column); }
	template<class KEY,class VALUE,class VARIANT_TYPE,class KEY_CAST_TYPE> QMap<KEY,VALUE> groupedBySingle( const QString & column ) const;
	template<class KEY,class VALUE> QMap<KEY,VALUE> groupedBySingle( const QString & column ) const { return groupedBySingle<KEY,VALUE,KEY,KEY>(column); }

	template<class KEY,class LIST> QMap<KEY,LIST> groupedByForeignKey( const QString & column ) const;
	QMap<Record,RecordList> groupedByForeignKey( const QString & column ) const { return groupedByForeignKey<Record,RecordList>(column); }
	
	/// Sorts the list according the the value in column.
	RecordList sorted( const QString & column, bool asc = true ) const;

	/// Returns a new list with all duplicate records removed.
	RecordList unique() const;

	/// Returns a new list with the same contents as this, in reversed order.
	RecordList reversed() const;
	
	/// If destTable is null then the copy is made from the current record's table
	/// If destTable has a different TableSchema than the current record, then each
	/// field is copied by database name, and if no match is found then by method name
	/// If updateCopiedRelations is true, then for each record that has a foreignKey
	/// pointing to another record in this list, the foreign key of the copied record
	/// will be updated to point to the corrosponding copied record in the return list.
	/// For example if you have list [ RecordA(fkeyB -> RecordB), RecordB ]
	RecordList copy( Table * destTable = 0, bool updateCopiedRelations = false );

	/// Calls Record::reload() on each record in this list.
	void reload();

	/// Returns a new list, with the contents of this list after calling
	/// Record::reload() on each record.
	RecordList reloaded() const;

	QString debug() const;

	/// print human readable dump of the records
	QString dump() const;

	typedef RecordIter Iter;
	// for Qt foreach compat
	typedef RecordIter const_iterator;

	// Updates each of the RecordImp *'s in the list to point to the current value
	// defined by Record::current(). 
	void makeCurrent();
protected:

	friend class RecordIter;
	friend class ChangeSet;
	
	class Private;

	/// Internally detaches this list by making a full copy
	void detach( Table * table = 0 );

	Private * d;
	RecordList( const ImpList & , Table * table = 0 );
private:
	static ImpList sEmptyList;
};

/**
 *  \class RecordIter
 *
 *  An easy way to iterate through RecordLists.
 *  Not quite compatible with a QList Iterator, but
 *  compatible with q_foreach macro.
 *
 *  \ingroup Stone
 */
class STONE_EXPORT RecordIter
{
public:
	RecordIter();
	RecordIter( const RecordList &, bool end=false );
	RecordIter( const ImpIter & );

	Record operator * () const;

	RecordImp * imp();

	RecordIter & operator=(const RecordIter &);
	RecordIter & operator=(const Record &);
	
	bool operator==( const RecordIter & );
	bool operator!=( const RecordIter & );

	void operator++();
	void operator--();

	void make_valid( RecordList );
	
	friend class RecordList;

protected:
	ImpIter mIter;
};

template<class KEY,class LIST,class VARIANT_TYPE,class KEY_CAST_TYPE> QMap<KEY,LIST> RecordList::groupedBy( const QString & column ) const
{
	QMap<KEY,LIST> ret;
	foreach( Record r, (*this) ) {
		QVariant v = r.getValue(column);
		ret[KEY_CAST_TYPE(v.value<VARIANT_TYPE>())] += r;
	}
	return ret;
}

template<class KEY,class VALUE,class VARIANT_TYPE,class KEY_CAST_TYPE> QMap<KEY,VALUE> RecordList::groupedBySingle( const QString & column ) const
{
	QMap<KEY,VALUE> ret;
	foreach( Record r, (*this) ) {
		QVariant v = r.getValue(column);
		ret[KEY_CAST_TYPE(v.value<VARIANT_TYPE>())] = r;
	}
	return ret;
}

template<class KEY,class LIST> QMap<KEY,LIST> RecordList::groupedByForeignKey( const QString & column ) const
{
	QMap<KEY,LIST> ret;
	foreach( Record r, (*this) ) {
		ret[KEY(r.foreignKey(column))] += r;
	}
	return ret;
}

///@}

} // namespace Stone

using Stone::RecordList;
using Stone::RecordIter;
using Stone::ImpIter;
using Stone::ConstImpIter;

Q_DECLARE_METATYPE(RecordList)

#endif // RECORD_LIST_H

