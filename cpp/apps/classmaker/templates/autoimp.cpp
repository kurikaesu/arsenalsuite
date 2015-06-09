
#include <qvariant.h>

#include "database.h"
#include "field.h"

#include "tl__.h"
#include "tl__imp.h"
#include "tl__table.h"
#include "bl__imp.h"
<%ELEMENTHEADERS%>
<%CLASSHEADERS%>

#define COMMIT_CODE
#define HEADER_FILES
<%BASEFUNCTIONS%>
#undef HEADER_FILES
#undef COMMIT_CODE

t__Imp::t__Imp()
<%MEMBERCTORS%>
{}

int t__Imp::set( QVariant * v )
{
	int i = b__Imp::set( v );
<%SETCODE%>
	return i;
}

int t__Imp::get( QVariant * v )
{
	int i = b__Imp::get( v );
<%GETCODE%>
	return i;
}


RecordImp * t__Imp::copy()
{
	t__Imp * t = new t__Imp;
	*t = *this;
	t->mKey = 0;
	t->mRefCount = 0;
	t->mUpdated = 0;
	return t;
}

QVariant t__Imp::getColumn( int col ) const
{
	Table * par = t__Table::instance()->parent();
	int cs = par ? (int)par->columnCount() : 0;
	if( col < cs )
		return b__Imp::getColumn( col );
<%GETCOLUMNCODE%>
	return QVariant();
}

void t__Imp::setColumn( int col, const QVariant & val )
{
	Table * par = t__Table::instance()->parent();
	int cs = par ? (int)par->columnCount() : 0;
	if( col < cs ) {
		b__Imp::setColumn( col, val );
		return;
	}
<%SETCOLUMNCODE%>
}

uint t__Imp::getUIntColumn( int col ) const
{
	return getColumn( col ).toUInt();
}

void t__Imp::setUIntColumn( int col, uint val )
{
	setColumn( col, QVariant( val ) );
}


Table * t__Imp::table() const
{
	return t__Table::instance();
}

void t__Imp::commit(bool newPrimaryKey)
{
	if( mDeleted )
		return;
	if( newPrimaryKey && mUpdated ){
		table()->update( this );
<%ELEMENTCOMMITCODE%>
	} else if( !newPrimaryKey || !mKey ) {
		table()->insert( this, newPrimaryKey );
#define COMMIT_CODE
<%BASEFUNCTIONS%>
#undef COMMIT_CODE
	}
}

