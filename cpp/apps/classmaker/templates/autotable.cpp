
#include <qvariant.h>

#include "snl__.h"
#include "field.h"

#include "tl__.h"
#include "tl__table.h"

#include "bl__table.h"

<%CLASSHEADERS%>
<%ELEMENTHEADERS%>

t__Schema * t__Schema::mSelf = 0;

t__Schema::t__Schema( Schema * schema )
: TableSchema( schema )
<%INDEXCTORS%>
{
<%SETPARENT%>	setTableName(__tu___TABLE__);
	setClassName(__tu___CLASS__);
	setUseCodeGen( true );
<%SETWHERE%><%ADDFIELDS%><%ADDINDEXCOLUMNS%><%PRELOAD%>
#define TABLE_CTOR
<%BASEHEADER%>
#undef TABLE_CTOR
}

t__Schema::~t__Schema()
{
	if( mSelf == this )
		mSelf = 0;
}

t__ * t__Schema::createObject( const Record & r )
{
	return new t__( r );
}

t__ * t__Schema::newObject()
{
	return new t__();
}

t__Schema * t__Schema::instance()
{
	if( !mSelf ){
		mSelf = new t__Schema( snl__Schema() );
	}
	return mSelf;
}

<%INDEXFUNCTIONS%>


