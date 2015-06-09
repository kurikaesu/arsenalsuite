
%MappedType MappedRecord
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#define MappedRecord Record
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new Record(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp );
%End
};

class Record
{

%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#undef arglist
%End

/*
%ConvertToSubClassCode
		sipClass = 0;
		Table * t = sipCpp->table();
		if( t ) {
			TableSchema * ts = t->schema();
			QPair<QString,QString> scm = getSchemaCastModule(ts->schema());
			if( ! scm.first.isEmpty() ) {
				QString type = ts->className();
				// This is very tricky.  We are actually casting up a Record object
				// This only works because all record subclasses contain NO extra
				// data, and there are no virtual functions.
				sipClass = getRecordWrapper( scm.first.toLatin1().constData(), scm.second + type );
			}
		}
%End
*/
public:
	Record( const Record & r );
	Record( RecordImp * /Transfer/, bool notused = false );
	Record( Table * table = 0 );

	~Record();

	MappedRecord getVersion( const ChangeSet & cs ) const;
	MappedRecord pristine() const;

	bool operator==( const Record & other ) const;
	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = (long)sipCpp->key();
%End

/*
	SIP_PYOBJECT __getattr__( const QString & attrName );
%MethodCode
		LOG_5( "attrName = " + *a0 );
		Table * t = sipCpp->table();
		QVariant ret;
		do {
			if( t ) {
				TableSchema * ts = t->schema();
				if (ts && ts->field(*a0)) {
					ret = sipCpp->getValue(*a0);
					break;
				}
			}
			sipIsErr = 1;
		} while (0);
		if( !sipIsErr ) {
			sipRes = wrapQVariant(ret,false,false);
			sipIsErr = (sipRes==0);
		}
%End

	SIP_PYOBJECT __setattr__( const QString & attrName, SIP_PYOBJECT value );
%MethodCode
		Table * t = sipCpp->table();
		QVariant ret;
		do {
			if( t ) {
				TableSchema * ts = t->schema();
				if (ts && ts->field(*a0)) {
					sipCpp->setValue(*a0,unwrapQVariant(a1));
					break;
				}
			}
			sipIsErr = 1;
		} while (0);
%End
*/

	SIP_PYOBJECT __repr__();
%MethodCode
	PyObject * mod = PyObject_GetAttrString( sipSelf, "__module__" );
	if( !mod ) {
		printf( "Class object had no __module__ attr\n" );
		Py_INCREF(Py_None);
		return Py_None;
	}
	if( !PyString_Check(mod) ) {
		printf( "__module__ attribute is not a string as expected\n" );
		Py_INCREF(Py_None);
		return Py_None;
	}
	PyObject * ret = PyString_FromFormat("%s.%s(%i)", PyString_AsString(mod), sipSelf->ob_type->tp_name, sipCpp->key());
	Py_DECREF(mod);
	return ret;
%End

	SIP_PYOBJECT __str__();
%MethodCode
	return PyString_FromFormat("<%s @%p *%i (%s)>",
			 sipSelf->ob_type->tp_name, sipCpp->imp(), sipCpp->key(), sipCpp->displayName().toLatin1().constData());
%End

	bool isValid() const;

	bool isRecord() const;

	ChangeSet changeSet() const;

	unsigned int key( bool generate = false ) const throw(SqlException,LostConnectionException,PythonException);

	QVariant getValue( const QString & column ) const throw(SqlException,LostConnectionException,PythonException);

	MappedRecord & setValue( const QString & column, const QVariant & value );

	MappedRecord & setColumnLiteral( const QString & column, const QString & literal );
	
	MappedRecord & setForeignKey( const QString & column, const Record & other );
	MappedRecord & setForeignKey( int column, const Record & other );
	MappedRecord & setForeignKey( Field * field, const Record & other );

	MappedRecord foreignKey( const QString & column ) const throw(SqlException,LostConnectionException,PythonException);
	MappedRecord foreignKey( int column ) const throw(SqlException,LostConnectionException,PythonException);
        MappedRecord foreignKey( Field * field ) const throw(SqlException,LostConnectionException,PythonException) /HoldGIL/;

	QString columnLiteral( const QString & column ) const;

	SIP_PYOBJECT getPyValue( Field * field ) const throw(SqlException,LostConnectionException,PythonException);
%MethodCode
	return wrapQVariant(sipCpp->getValue(a0),false,true,a0->qvariantType());
%End
	
	QVariant getValue( int column ) const throw(SqlException,LostConnectionException,PythonException);
	QVariant getValue( Field * field ) const throw(SqlException,LostConnectionException,PythonException);

	MappedRecord & setValue( int column, const QVariant & value );
	MappedRecord & setValue( Field * field, const QVariant & value );

	QString displayName() const throw(SqlException,LostConnectionException,PythonException);
	QString stateString() const;
	QString debug() const;
	QString dump() const;

	QString changeString() const;

	void selectFields( QList<Field*> fields = QList<Field*>(), bool refreshExisting = false ) throw(SqlException,LostConnectionException,PythonException);

	int remove() throw(SqlException,LostConnectionException,PythonException);

	MappedRecord & reload( bool lockForUpdate=false ) throw(SqlException,LostConnectionException,PythonException);

	MappedRecord & commit() throw(SqlException,LostConnectionException,PythonException);

	RecordImp * imp() const;

	bool isUpdated( Field * f = 0 ) const;

	Table * table() const;

	MappedRecord copy(Table * destTable = 0) const;

	static int totalRecordCount();
	static int totalRecordImpCount();
};

