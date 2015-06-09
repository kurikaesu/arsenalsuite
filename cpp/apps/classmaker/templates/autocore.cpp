

#include "tl__.h"
#include "tl__table.h"

<%CLASSHEADERS%>
<%ELEMENTHEADERS%>
<%INDEXHEADERS%>

<%TABLEDATA%>

t__::t__()
: b__( t__::table()->emptyImp(), false )
{
}

t__::t__( uint key )
: b__( table()->record( key ).mImp, false )
{
	if( !mImp )
		*this = t__();
}

void t__::checkImpType()
{
	Record::checkImpType( t__Schema::instance() );
}

t__ & t__::operator=( const t__ & other ) {
	Record::operator=( other );
	checkImpType();
	return *this;
}

<%METHODS%>
<%INDEXMETHODS%>
<%ELEMENTMETHODS%>
t__List t__::select( const QString & where, const VarList & args )
{
	return table()->select( where, args );
}

t__List t__::select( const Expression & exp )
{
	return table()->select( exp );
}

Table * t__::table() {
	return t__Schema::instance()->table();
}

t__Schema * t__::schema() {
	return t__Schema::instance();
}

struct t__::_c t__::c = {
	<%SCHEMAFIELDDEFS%>
};
