
#include "tl__.h"
#include "tl__table.h"

<%CLASSHEADERS%>

t__List::t__List()
{
}

t__List::t__List( const t__List & other )
: b__List( other )
{
}

t__List::t__List( const RecordList & other, Table * table )
: b__List( other, table ? table : t__::table() )
{}

t__List::t__List( const ImpList & other, Table * table )
: b__List( other, table ? table : t__::table() )
{}

t__List::t__List( const Record & rec, Table * table )
: b__List( rec, table ? table : t__::table() )
{}

t__List & t__List::operator=( const RecordList & other )
{
	RecordList::operator=( other );
	return *this;
}

t__ t__List::operator []( uint i ) const
{
	return t__( RecordList::operator[](i) );
}

t__List & t__List::operator += ( const Record & t )
{
	RecordList::operator+=( t );
	return *this;
}

t__List & t__List::operator += ( const RecordList & list )
{
	RecordList::operator+=( list );
	return *this;
}

void t__List::insert( t__Iter it, const t__ & t )
{
	RecordList::insert( RecordIter( it.mIter ), t );
}

t__Iter t__List::at( uint pos )
{
	return (d && (int)pos < d->mList.size()) ? t__Iter( d->mList.begin() + pos ) : end();
}

t__List t__List::slice( int start, int end, int step )
{
	return t__List(RecordList::slice(start,end,step));
}

t__Iter t__List::find( const Record & val )
{
	return d ? t__Iter( d->mList.begin() + d->mList.indexOf( val.imp() ) ) : end();
}

t__Iter t__List::remove( const t__Iter & it )
{
	if( d ){
		detach();
		if( it.mIter != d->mList.end() ){
			if( *it.mIter )
				(*it.mIter)->deref();
			return t__Iter( d->mList.erase( it.mIter ) );
		}
	}
	return end();
}

Table * t__List::table() const
{
	return t__::table();
}

TableSchema * t__List::schema() const
{
	return t__Schema::instance();
}

t__Iter t__List::begin() const
{
	return t__Iter( *this );
}

t__Iter t__List::end() const
{
	return t__Iter( *this, true );
}

<%LISTMETHODS%>

t__Iter::t__Iter()
: RecordIter()
{}

t__Iter::t__Iter( ImpIter it )
: RecordIter( it )
{}

t__Iter::t__Iter( const t__List & list, bool end )
: RecordIter( list, end )
{}

t__ t__Iter::operator * ()
{
	return t__( *mIter );
}

t__Iter & t__Iter::operator=(const t__Iter & iter )
{
	mIter = iter.mIter;
	return *this;
}

t__Iter & t__Iter::operator=(const Record & r)
{
	r.imp()->ref();
	(*mIter)->deref();
	*mIter = r.imp();
	return *this;
}
