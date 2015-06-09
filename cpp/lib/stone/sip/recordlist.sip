
%MappedType MappedRecordList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#define MappedRecordList RecordList
%End

// We are never going to use MappedRecord as a c++ method argument type
// only as a return type, so ConvertToTypeCode implementation isn't needed
%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp );
%End
};

class RecordList
{
%TypeHeaderCode
#include <recordlist.h>
%End
public:
	RecordList();
	RecordList( const RecordList & other, Table * table );
	RecordList( const RecordList & other );
	RecordList( const MappedRecord & rec, Table * table = 0 );
	RecordList( SIP_PYLIST );
%MethodCode
	sipCpp = new RecordList( recordListFromPyList(a0) );
%End

	MappedRecordList & operator += ( const Record & );
	MappedRecordList & operator += ( const RecordList & );

	MappedRecordList operator + ( const RecordList & ) const;

	MappedRecordList & operator -= ( const RecordList & );
	MappedRecordList operator -= ( const Record & );

	MappedRecordList operator - ( const RecordList & ) const;

	// Returns the intersection of the two lists
	MappedRecordList operator & ( const RecordList & ) const;

	// Sets this as the intersection between this and other
	// returns a reference to this
	MappedRecordList & operator &= ( const RecordList & other );
	
	// Returns the union of this and other
	MappedRecordList operator | ( const RecordList & other ) const;
	
	// Sets this to the union of this and other
	// Returns a reference to this
	MappedRecordList & operator |= ( const RecordList & other );
	
	bool operator==( const RecordList & ) const;
	bool operator!=( const RecordList & list ) const;

//	RecordIter at( uint ) const;
//	RecordImp * imp( uint ) const;

	void reverse();

	int findIndex( const Record & );
	int index( const Record & );
%MethodCode
		int idx = sipCpp->findIndex(*a0);
		if( idx < 0 ) {
			PyErr_SetString(PyExc_ValueError, "RecordList.index(x): x not in list");
			sipIsErr = 1;
		}
%End

	void extend( const RecordList & other );
%MethodCode
		*sipCpp += *a0;
%End

	void extend( SIP_PYOBJECT );
%MethodCode
		if( PySequence_Check(a0) ) {
			SIP_SSIZE_T end = PySequence_Size(a0);
			for( SIP_SSIZE_T i = 0; i < end; i++ ) {
				PyObject * ob = PySequence_GetItem(a0,i);
				bool cc = ob && sipCanConvertToType( ob, sipType_Record, 0 );
				Py_XDECREF(ob);
				if( !cc ) {
					sipIsErr = 1;
					PyErr_SetString(PyExc_TypeError, "RecordList.extend argument must be a sequence of Record classes" );
					break;
				}
			}
			if( sipIsErr == 0 ) {
				for( SIP_SSIZE_T i = 0; i < end; i++ ) {
					PyObject * ob = PySequence_GetItem(a0,i);
					int state, err = 0;
					Record * r = (Record*)sipConvertToType( ob, sipType_Record, 0, 0, &state, &err );
					if( r && !err ) {
						sipCpp->append( *r );
						sipReleaseType( r, sipType_Record, state );
					}
					Py_XDECREF(ob);
				}
			}
		} else {
			sipIsErr = 1;
			PyErr_SetString(PyExc_TypeError, "object is not iterable" );
		}
%End

//	RecordIter find( const Record & );

	void append( const Record & );
	
	void insert( int pos, const Record & r );
//	void insert( RecordIter it, const Record & r );
	
	int remove( const Record & );
%MethodCode
		sipRes = sipCpp->remove(*a0);
		if( sipRes < 0 ) {
			sipIsErr = 1;
			PyErr_SetString(PyExc_ValueError, "RecordList.remove(x): x not in list");
		}
%End

//	int remove( RecordImp * );

	MappedRecord pop( int i = -1 );

//	RecordIter remove( const RecordIter & );

	void clear();

	bool contains( const Record & ) const;
//	bool contains( RecordImp * ) const;

	bool isEmpty() const;

	unsigned int size() const;
	unsigned int count() const;
    int __len__();
%MethodCode
	sipRes = sipCpp->count();
%End

	MappedRecord operator[](int i);
%MethodCode
	int len;
	
	len = sipCpp->count();
	
	if ((a0 = sipConvertFromSequenceIndex(a0, len)) < 0)
		sipIsErr = 1;
	else
		sipRes = new Record((*sipCpp)[a0]);
%End

	MappedRecordList __getitem__( SIP_PYSLICE );
%MethodCode
	SIP_SSIZE_T start, stop, step, slicelen;
	if( sipConvertFromSliceObject( a0, sipCpp->size(), &start, &stop, &step, &slicelen ) == 0 ) {
		sipRes = new RecordList(sipCpp->slice(start,stop,step));
	} else
		sipIsErr = 1;
%End


	void __delitem__( int index );
%MethodCode
		sipCpp->pop(a0);
%End

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
	PyObject * ret = PyString_FromFormat("%s.%s( %s.%s().table().records([%s]) )", PyString_AsString(mod), sipSelf->ob_type->tp_name, PyString_AsString(mod), sipSelf->ob_type->tp_name, sipCpp->keyString().toLatin1().constData() );
	Py_DECREF(mod);
	return ret;
%End

	SIP_PYOBJECT __str__();
%MethodCode
	PyObject *s, *temp;
	PyObject *pieces = NULL;

	if (sipCpp->size() == 0) {
		return PyString_FromFormat("<%s object at %p>",
					 sipSelf->ob_type->tp_name, sipSelf);
	}

	pieces = PyList_New(0);
	if (pieces == NULL)
		goto Done;

	/* Do repr() on each element. */
	for (int i = 0; i < (int)sipCpp->size(); ++i) {
		int status;
		PyObject * item = PySequence_GetItem(sipSelf,i);
		if( item == NULL )
			goto Done;
		s = PyObject_Repr(item);
		Py_DECREF(item);
		if (s == NULL)
			goto Done;
		status = PyList_Append(pieces, s);
		Py_DECREF(s);  /* append created a new ref */
		if (status < 0)
			goto Done;
	}

	/* Add "[]" decorations to the first and last items. */
	assert(PyList_GET_SIZE(pieces) > 0);
	s = PyString_FromFormat("<%s object at %p> [", sipSelf->ob_type->tp_name, sipSelf);
	if (s == NULL)
		goto Done;
	temp = PyList_GET_ITEM(pieces, 0);
	PyString_ConcatAndDel(&s, temp);
	PyList_SET_ITEM(pieces, 0, s);
	if (s == NULL)
		goto Done;

	s = PyString_FromString("]");
	if (s == NULL)
		goto Done;
	temp = PyList_GET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1);
	PyString_ConcatAndDel(&temp, s);
	PyList_SET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1, temp);
	if (temp == NULL)
		goto Done;

	/* Paste them all together with ", " between. */
	s = PyString_FromString(", ");
	if (s == NULL)
		goto Done;
	sipRes = _PyString_Join(s, pieces);
	Py_DECREF(s);
Done:
	Py_XDECREF(pieces);
%End

	void selectFields( QList<Field*> fields = QList<Field*>(), bool refreshExisting = false ) throw(SqlException,LostConnectionException,PythonException);

	void commit() throw(SqlException,LostConnectionException,PythonException);
	int remove() throw(SqlException,LostConnectionException,PythonException);

	QList<QVariant> getValue( const QString & column ) throw(SqlException,LostConnectionException,PythonException);
	void setValue( const QString & column, const QVariant & value );

	QList<QVariant> getValue( int column ) throw(SqlException,LostConnectionException,PythonException);
	QList<QVariant> getValue( Field * f ) const throw(SqlException,LostConnectionException,PythonException);
	void setValue( int column, const QVariant & value );
	void setValue( Field * f, const QVariant & value );

	QString keyString() const;

	MappedRecordList foreignKey( int column, int lookupMode = 0x7 ) const throw(SqlException,LostConnectionException,PythonException);
	MappedRecordList foreignKey( const QString & column, int lookupMode = 0x7 ) const throw(SqlException,LostConnectionException,PythonException);
	MappedRecordList foreignKey( Field * f, int lookupMode = 0x7 ) const throw(SqlException,LostConnectionException,PythonException);

	MappedRecordList setForeignKey( int column, const Record & fkey );
	MappedRecordList setForeignKey( const QString & column, const Record & fkey );
	MappedRecordList setForeignKey( Field * f, const Record & fkey );

	void setColumnLiteral( const QString & column, const QString & value );

	QList<uint> keys( int idx=-1 ) const;

//	RecordIter begin() const;

//	RecordIter end() const;

	Table * table() const;

	MappedRecordList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const throw(SqlException,LostConnectionException,PythonException);
	MappedRecordList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const throw(SqlException,LostConnectionException,PythonException);
	MappedRecordList filter( const Expression &, bool keepMatches = true ) const throw(SqlException,LostConnectionException,PythonException);

	QMap<QString,RecordList> groupedBy( const QString & column ) const throw(SqlException,LostConnectionException,PythonException);

	//QMap<QVariant,MappedRecordList> groupedBy( const QString & column ) const throw(SqlException,LostConnectionException,PythonException);
	QMap<MappedRecord,MappedRecordList> groupedByForeignKey( const QString & column ) const throw(SqlException,LostConnectionException,PythonException);

	PyObject * groupedBy( SIP_PYCALLABLE callable ) const throw(SqlException,LostConnectionException,PythonException) /HoldGIL/;
%MethodCode
		sipRes = recordListGroupByCallable( sipCpp, a0 );
%End

	void sort() /NoArgParser/;
%MethodCode

	//PyErr_Warn( PyExc_Warning, "entering RecordList.sort" );
	if( !sipCanConvertToType( sipSelf, sipType_RecordList, SIP_NO_CONVERTORS | SIP_NOT_NONE ) ) {
		PyErr_SetString( PyExc_ValueError, "RecordList not passed as self" );
		return 0;
	}

	PyObject * builtins = PyEval_GetBuiltins();
	PyObject * sorted = PyDict_GetItemString(builtins,"sorted");

	if( !sorted || !PyCallable_Check(sorted) ) {
		PyErr_SetString( PyExc_RuntimeError, "Unable to find callable sorted method in builtins" );
		return 0;
	}
	
	PyObject * callList = PyTuple_New(PySequence_Size(sipArgs) + 1);
	Py_INCREF( sipSelf );
	PyTuple_SetItem( callList, 0, sipSelf );
	for( int i = PySequence_Size(sipArgs); i > 0; i-- )
		PyTuple_SetItem(callList, i, PySequence_GetItem( sipArgs, i-1 ));

	//PyErr_Warn( PyExc_Warning, "Calling sorted" );
	PyObject * sorted_list = PyObject_Call( sorted, callList, sipKwds );
	Py_DECREF(callList);

	if( !sorted_list )
		return 0;

	if( !PySequence_Check( sorted_list ) ) {
		PyErr_SetString( PyExc_RuntimeError, "sorted method returned a non sequence" );
		Py_DECREF(sorted_list);
		return 0;
	}

	int state, err = 0;
	RecordList * rl = (RecordList*)sipConvertToType( sipSelf, sipType_RecordList, 0, SIP_NO_CONVERTORS | SIP_NOT_NONE, &state, &err );

	if( err ) {
		Py_DECREF(sorted_list);
		return 0;
	}

	rl->clear();
	
	//PyErr_Warn( PyExc_Warning, "Calling RecordList.extend" );
	PyObject * ret = PyObject_CallMethod( sipSelf, "extend", "(O)", sorted_list );
	if( !ret ) return 0;

	sipReleaseType( rl, sipType_RecordList, state );
	Py_DECREF(ret);
	Py_DECREF(sorted_list);

	Py_INCREF(Py_None);
	return Py_None;
%End

	MappedRecordList sorted( const QString & column, bool asc = true ) const throw(SqlException,LostConnectionException,PythonException);

	MappedRecordList unique() const;

	MappedRecordList reversed() const;

	MappedRecordList copy( Table * destTable = 0, bool updateCopiedRelations = false );
	
	void reload() throw(SqlException,LostConnectionException,PythonException);

	MappedRecordList reloaded() const throw(SqlException,LostConnectionException,PythonException);

	QString debug() const;

	QString dump() const;
};

