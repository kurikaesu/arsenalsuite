
#ifndef tu___H
#define tu___H

#include <qstring.h>
#include <qdatetime.h>
#include <qlist.h>
#include <qvariant.h>
#include <qimage.h>

#include "snl__.h"
#include "expression.h"
#include "interval.h"
#include "joinedselect.h"
#include "table.h"
#include "bl__.h"

using namespace Stone;

#define HEADER_FILES
<%BASEHEADER%>
#undef HEADER_FILES

class t__Schema;
class t__List;

typedef QList<QVariant> VarList;

/// @cond
<%CLASSDEFS%>
/// @endcond

<%CLASSDOCS%>
class snu___EXPORT t__ : public b__
{
public:
	/*
	 * Default constructor.  Creates a valid, uncommited t__ record.
	 */
	t__();

	/*
	 * Looks up the t__ record with primary key \param key
	 */
	t__( uint key );

	/*
	 * Constructs a shallow copy of \param other
	 */
	t__( const t__ & other ) : b__( other ) {}

	/*
	 * Constructs a shallow copy of \param r
	 * If r is not a derived from t__, then
	 * the record will be invalid.
	 */
	t__( const Record & r )
	: b__( r.mImp, false )
	{
		checkImpType();
	}

	t__( RecordImp * imp, bool checkType = true )
	: b__( imp, false )
	{
		if( checkType ) checkImpType();
	}
	
	/*
	 * Returns a copy of this record, with the primary key
	 * set to 0.
	 */
	t__ copy() const { return t__( Record::copy() ); }

	t__ & operator=( const t__ & other );


<%METHODDEFS%>

	static t__List select( const QString & where = QString(), const VarList & args = VarList() );
	static t__List select( const Expression & exp );
	// Usage
	// Employee::join<UserGroup>().join<Group>().select()
	template<typename T> static JoinedSelect join( QString condition = QString(), JoinType joinType = InnerJoin, bool ignoreResults = false, const QString & alias = QString() )
	{ return table()->join( T::table(), condition, joinType, ignoreResults, alias ); }
	
<%INDEXDEFS%>

<%ELEMENTHACKS%>

	struct _c {
<%SCHEMAFIELDDECLS%>
	};
	static _c c;

#define CLASS_FUNCTIONS
<%BASEHEADER%>
#undef CLASS_FUNCTIONS

	static Table * table();
	static t__Schema * schema();
private:
	void checkImpType();
};

#include "tl__list.h"
#include "tl__table.h"

#endif // tu___H

