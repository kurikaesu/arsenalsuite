
%MappedType Mappedt__
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "tl__list.h"
#include "tl__table.h"
#define Mappedt__ t__
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new t__(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, t__Schema::instance() );
%End
};

class t__ : b__
{
%TypeHeaderCode
#include <tl__.h>
<%CLASSHEADERS%>
%End
public:
	t__() /HoldGIL/;

	t__( uint key ) throw(SqlException,LostConnectionException,PythonException);

	t__( const t__ & other ) /HoldGIL/;

	t__( const Record & r ) /HoldGIL/;

	t__( RecordImp * imp, bool checkType = true ) /HoldGIL/;
	
	Mappedt__ copy() const /HoldGIL/;

	bool operator==( const Record & other ) const /HoldGIL/;

	bool operator!=( const Record & other ) const /HoldGIL/;
	
	bool operator <( const Record & other ) const /HoldGIL/;

	long __hash__() const /HoldGIL/;
%MethodCode
		sipRes = sipCpp->key();
%End

<%METHODDEFS%>

	static Mappedt__List select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() ) throw(SqlException,LostConnectionException,PythonException);
	static Mappedt__List select( const Expression & ) throw(SqlException,LostConnectionException,PythonException);

	static JoinedSelect join( SIP_PYOBJECT cls, QString condition = QString(), JoinType joinType = InnerJoin, bool ignoreResults = false, const QString & alias = QString() );
%MethodCode
		// Convert to python
		Table * inst = 0;

		if( !sipCanConvertToType(a0,sipType_Table,SIP_NOT_NONE) ) {
			// Attempt to get the table * by calling the static method table()
			if( !sipIsErr && !inst ) {
				PyObject * pyTableMethod = PyObject_GetAttrString( a0, "table" );
				if( pyTableMethod ) {
					PyObject * pyTable = PyObject_CallObject( pyTableMethod, 0 );
					if( pyTable && sipCanConvertToType(pyTable,sipType_Table,SIP_NOT_NONE) )
						inst = (Table*)sipConvertToType(pyTable,sipType_Table,0,SIP_NOT_NONE,0,&sipIsErr);
					Py_XDECREF(pyTable);
					Py_XDECREF(pyTableMethod);
				} else
					sipIsErr = 1;
			}
		}
		
		if( !inst && !sipIsErr && a0 && sipCanConvertToType(a0,sipType_Table,SIP_NOT_NONE) ) {
			inst = (Table*)sipConvertToType(a0,sipType_Table,0,SIP_NOT_NONE,0,&sipIsErr);
		}

		if( !sipIsErr ) {
			sipRes = new JoinedSelect(t__::table()->join( inst, *a1, a2, a3, *a4 ));
		}
%End

	Mappedt__ reload( bool lockForUpdate = false ) throw(SqlException,LostConnectionException,PythonException);

<%INDEXDEFS%>

<%ELEMENTHACKS%>

<%BASEHEADER%>

	struct _c {
<%SCHEMAFIELDDECLS%>
	private:
		_c();
	};
	static _c c;

	static Table * table() /HoldGIL/;
	//static t__Schema * schema();
};


