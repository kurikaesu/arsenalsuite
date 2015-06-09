
#ifndef tu___IMP_H
#define tu___IMP_H

#include <qstring.h>
#include <qmap.h>
#include <qdatetime.h>

#include "snl__.h"
#include "index.h"

#include "bl__imp.h"

#define IMP_HEADER_FILES
<%BASEHEADER%>
#undef IMP_HEADER_FILES

#include "tl__.h"

class QVariant;

class snu___EXPORT t__Imp : public b__Imp
{
public:
	t__Imp();

	virtual int get( QVariant * v );
	virtual int set( QVariant * v );

	virtual QVariant getColumn( int col ) const;
	virtual void setColumn( int col, const QVariant & v );

	virtual uint getUIntColumn( int col ) const;
	virtual void setUIntColumn( int col, uint val );

	virtual RecordImp * copy();

	virtual void commit(bool newPrimaryKey=true);

<%ELEMENTHACKS%>

	virtual Table * table() const;

<%MEMBERVARS%>


#define IMP_CLASS_FUNCTIONS
<%BASEHEADER%>
#undef IMP_CLASS_FUNCTIONS

};

#endif // tu___IMP_H

