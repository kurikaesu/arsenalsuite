
#ifndef tu___LIST_H
#define tu___LIST_H

#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>

#include "snl__.h"
#include "interval.h"
#include "bl__list.h"
#include "recordlist_p.h"

namespace Stone {
class Record;
class RecordImp;
class TableSchema;
};

using namespace Stone;

class t__;
class t__Iter;
<%CLASSDEFS%>

class snu___EXPORT t__List : public b__List
{
public:
	t__List();
	t__List( const RecordList &, Table * t = 0 );
	t__List( const t__List & );
	t__List( const Record &, Table * t = 0 );
	
	virtual ~t__List(){}

	t__List & operator=( const RecordList & );

	t__ operator []( uint ) const;

	t__List & operator += ( const Record & );
	t__List & operator += ( const RecordList & );

	void insert( t__Iter, const t__ & );

	t__Iter at( uint );

	t__List slice( int start, int end = INT_MAX, int step = 1 );
	
	t__Iter find( const Record & );

	using b__List::remove;
	t__Iter remove( const t__Iter & );

	t__Iter begin() const;

	t__Iter end() const;

	t__List filter( const QString & column, const QVariant & value, bool keepMatches = true ) const
		{ return t__List( RecordList::filter( column, value, keepMatches ) ); }
	t__List filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const
		{ return t__List( RecordList::filter( column, re, keepMatches ) ); }
        t__List filter( const Expression & exp, bool keepMatches = true ) const
                { return t__List( RecordList::filter( exp, keepMatches ) ); }

	t__List sorted( const QString & c, bool a = true ) const { return t__List( RecordList::sorted( c, a ) ); }

		/// Returns a new list with all duplicate records removed.
	t__List unique() const { return t__List( RecordList::unique() ); }

	/// Returns a new list with the same contents as this, in reversed order.
	t__List reversed() const { return t__List( RecordList::reversed() ); }
	
	/// Returns a new list, with the contents of this list after calling
	/// Record::reload() on each record.
	t__List reloaded() const { return t__List( RecordList::reloaded() ); }

	virtual Table * table() const;
	TableSchema * schema() const;

<%LISTDEFS%>

	typedef t__Iter Iter;
	// for Qt foreach compat
	typedef t__Iter const_iterator;
	friend class t__Iter;
protected:
	t__List( const ImpList &, Table * t = 0 );

};

class snu___EXPORT t__Iter : public RecordIter
{
public:
	t__Iter();
	t__Iter( const t__List &, bool end=false );
	t__Iter( ImpIter );

	t__ operator * ();

	t__Iter & operator=(const t__Iter &);
	t__Iter & operator=(const Record &);

	friend class t__List;
};

#endif // tu___LIST_H

