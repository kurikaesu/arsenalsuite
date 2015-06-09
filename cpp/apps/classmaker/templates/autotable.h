
#include "tl__.h"

#ifndef tu___TABLE_H
#define tu___TABLE_H

#include "snl__.h"
#include "index.h"
#include "table.h"
#include "tl__list.h"
#include "expression.h"

class t__;
namespace Stone {
class Database;
}
using namespace Stone;

class snu___EXPORT t__Schema : public TableSchema
{
public:
	t__Schema( Schema * schema );
	~t__Schema();

	virtual t__ * createObject( const Record & );
	virtual t__ * newObject();
	
<%TABLEMEMBERS%>

#define TABLE_FUNCTIONS
<%BASEHEADER%>
#undef TABLE_FUNCTIONS

	/*snu___EXPORT*/ static t__Schema * instance();
	/*snu___EXPORT*/ static t__Schema * mSelf;
};

#endif // <%UPPERCLASS%>_IMP_H


