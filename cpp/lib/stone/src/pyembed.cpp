
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include "pyembed.h"

#include <qcolor.h>
#include <qdatetime.h>
#include <qmetatype.h>
#include <qimage.h>

#include "interval.h"
#include "tableschema.h"

PythonException::PythonException()
: type(0)
, value(0)
, traceback(0)
{
	SIP_BLOCK_THREADS
	PyErr_Fetch(&type, &value, &traceback);
	SIP_UNBLOCK_THREADS
}

PythonException::~PythonException() throw()
{
	Py_XDECREF(type);
	Py_XDECREF(value);
	Py_XDECREF(traceback);
}

void PythonException::restore()
{
	PyErr_Restore(type, value, traceback);
	type = value = traceback = 0;
}

static inline void ensurePythonInitialized()
{
	if( ! Py_IsInitialized() )
		Py_Initialize();
}

const sipAPIDef * getSipAPI()
{
	ensurePythonInitialized();

	static const sipAPIDef * api = 0;
	if( api ) return api;

	/* Import the SIP module and get it's API.
	 * libsip does not provide a symbol for accessing the sipAPIDef object
	 * it must be retrieved through sip's python module's dictionary
	 */
    SIP_BLOCK_THREADS
	PyObject * sip_sipmod = PyImport_ImportModule((char *)"sip");
	if (sip_sipmod == NULL) {
		LOG_3( "getSipAPI: Error importing sip module" );
	} else {

		PyObject * sip_capiobj = PyDict_GetItemString(PyModule_GetDict(sip_sipmod),"_C_API");
#if defined(SIP_USE_PYCAPSULE)
		if (sip_capiobj == NULL || !PyCapsule_CheckExact(sip_capiobj))
#else
		if (sip_capiobj == NULL || !PyCObject_Check(sip_capiobj))
#endif
			LOG_3( QString("getSipAPI: Unable to find _C_API object from sip modules dictionary: Got 0x%1").arg((qulonglong)sip_capiobj,0,16) );
		else
#if defined(SIP_USE_PYCAPSULE)
			api = reinterpret_cast<const sipAPIDef *>(PyCapsule_GetPointer(sip_capiobj, SIP_MODULE_NAME "._C_API"));
#else
			api = reinterpret_cast<const sipAPIDef *>(PyCObject_AsVoidPtr(sip_capiobj));
#endif
	}
    SIP_UNBLOCK_THREADS
	return api;
}
/*
sipExportedModuleDef * getSipModule( const char * name )
{
	const sipAPIDef * api = getSipAPI();
	if( !api ) return 0;

	sipExportedModuleDef * module = api->api_find_module( name );
	if( !module )
		LOG_5( "getSipModule: Unable to lookup module " + QString::fromLatin1(name) + " using api_find_module" );
	return module;
}
*/
sipTypeDef * getSipType( const char * /*module_name*/, const char * typeName )
{
	/*
	sipExportedModuleDef * module = getSipModule(module_name);
	if( !module ) return 0;

	for( int i = module->em_nrtypes - 1; i >= 0; i-- ) {
		sipTypeDef * type = module->em_types[i];
		// We could try checking type->u.td_py_type->name
		if( //strcmp( type->td_name, typeName ) == 0 ||
			 ( strcmp( sipNameFromPool( type->td_module, type->td_cname ), typeName ) == 0 ) )
			return type;
	}*/
	const sipTypeDef * type = getSipAPI()->api_find_type(typeName);
	if( type ) return const_cast<sipTypeDef*>(type);

	LOG_5( "getSipType: Unabled to find " + QString::fromLatin1(typeName) );// + " in module " + QString::fromLatin1(module_name) );
	return 0;
}

QMap<QPair<Table*,int>,CompiledPythonFragment> codeCache;

CompiledPythonFragment * getCompiledCode( Table * table, int pkey, const QString & pCode, const QString & codeName )
{
	CompiledPythonFragment * ret = 0;
	ensurePythonInitialized();
	// Create the key for looking up the code fragment in the cache
	QPair<Table*,int> codeKey = qMakePair(table,pkey);

	// Return the cached code fragment, if the code still matches
	if( codeCache.contains( codeKey ) ) {
		CompiledPythonFragment * frag = &codeCache[codeKey];
		// This should actually be very fast because of QString's
		// implicit sharing, should usually just be a pointer comparison
		if( frag->codeString == pCode )
			return frag;

		// Remove from cache if the fragment is out of date
		// TODO: This should free any references and remove the module
		// this isn't a big deal though, since code fragments won't
		// change very often during program execution
		codeCache.remove( codeKey );
	}

	if( pCode.isEmpty() )
		return 0;

	// Compile the code
	CompiledPythonFragment frag;
	frag.codeString = pCode;
	SIP_BLOCK_THREADS
	frag.code = (PyCodeObject*)Py_CompileString( pCode.toLatin1(), codeName.toLatin1(), Py_file_input );
	SIP_UNBLOCK_THREADS
	if( !frag.code )
	{
		PyErr_Print();
		LOG_5( "PathTemplate:getCompiledCode: Error Compiling Python code for: " + table->schema()->tableName() + " " + QString::number( pkey ) + " " + codeName );
		return 0;
	}

	// Load the code into a module
	// This is the only way i could figure out how to make the globals work properly
	// before i was using PyEval_EvalCode, passing the __main__ dict as the globals
	// but this wasn't working properly when calling the functions later, because
	// the import statements were lost.
	// This method works great and takes less code.
	
	// Generate a unique module name
	QString moduleName = "__" + table->schema()->tableName() + "__" + QString::number(pkey) + "__";
	// Returns a NEW ref
	SIP_BLOCK_THREADS
	PyObject * module = PyImport_ExecCodeModule(moduleName.toLatin1().data(),(PyObject*)frag.code);
	if( !module ) {
		PyErr_Print();
		LOG_3( "Unable to execute code module" );
	}

	// Save the modules dict, so we can lookup the function later
	else {
		frag.locals = PyModule_GetDict(module);
		Py_INCREF(frag.locals);
		codeCache[codeKey] = frag;
		ret = &codeCache[codeKey];
	}
	SIP_UNBLOCK_THREADS

	return ret;
}

CompiledPythonFragment * getCompiledCode( const Record & r, const QString & pCode, const QString & codeName )
{
	return getCompiledCode( r.table(), r.key(), pCode, codeName );
}

PyObject * getCompiledFunction( const char * functionName, Table * table, int pkey, const QString & pCode, const QString & codeName )
{
	CompiledPythonFragment * frag = getCompiledCode( table, pkey, pCode, codeName );
	if( !frag ) return 0;

	/// Returns a BORROWED reference
	PyObject * ret = 0;
	SIP_BLOCK_THREADS
	ret = PyDict_GetItemString( frag->locals, functionName );
	SIP_UNBLOCK_THREADS
	if( !ret ) {
		LOG_3( "getCompiledFunction: Unable to find function " + QString::fromLatin1(functionName) + " in code object" );
	}
	return ret;
}

PyObject * getCompiledFunction( const char * functionName, const Record & r, const QString & pCode, const QString & codeName )
{
	return getCompiledFunction( functionName, r.table(), r.key(), pCode, codeName );
}

static sipTypeDef * sipColorType()
{ static sipTypeDef * sColorW = 0; if( !sColorW ) sColorW = getSipType("PyQt4.QtGui", "QColor"); return sColorW; }

static sipTypeDef * sipImageType()
{ static sipTypeDef * sImageW = 0; if( !sImageW ) sImageW = getSipType("PyQt4.QtGui", "QImage"); return sImageW; }

static sipTypeDef * sipDateType()
{ static sipTypeDef * sDateW = 0; if( !sDateW ) sDateW = getSipType("PyQt4.QtCore","QDate"); return sDateW; }

static sipTypeDef * sipDateTimeType()
{ static sipTypeDef * sDateTimeW = 0; if( !sDateTimeW ) sDateTimeW = getSipType("PyQt4.QtCore","QDateTime"); return sDateTimeW; }

static sipTypeDef * sipTimeType()
{ static sipTypeDef * sTimeW = 0; if( !sTimeW ) sTimeW = getSipType("PyQt4.QtCore","QTime"); return sTimeW; }

static sipTypeDef * sipStringType()
{ static sipTypeDef * sStringW = 0; if( !sStringW ) sStringW = getSipType("PyQt4.QtCore","QString"); return sStringW; }

static sipTypeDef * sipQVariantType()
{ static sipTypeDef * sQVariantW = 0; if( !sQVariantW ) sQVariantW = getSipType("PyQt4.QtCore","QVariant"); return sQVariantW; }

static sipTypeDef * sipIntervalType()
{ static sipTypeDef * sIntervalW = 0; if( !sIntervalW ) sIntervalW = getSipType("blur.Stone", "Interval"); return sIntervalW; }

sipTypeDef * getRecordType( const char * module, const char * className )
{
	/*
	QString hash = QString::fromLatin1(module) + "." + className;
	static QHash<QString,sipTypeDef*> sipTypeHash;
	
	QHash<QString,sipTypeDef*>::iterator it = sipTypeHash.find( hash );
	if( it != sipTypeHash.end() )
		return it.value();
	*/
	sipTypeDef * ret = getSipType(module,className);
	return ret;
}

// Returns a BORROWED reference
PyObject * getClassObject( PyObject * pyObject )
{
	bool isInstance = false;
	SIP_BLOCK_THREADS
	isInstance = PyInstance_Check( pyObject );
	SIP_UNBLOCK_THREADS
	if( isInstance )
		return (PyObject*)(((PyInstanceObject*)pyObject)->in_class);
	return 0;
}

// Returns a BORROWED reference
PyObject * getPythonClass( const char * moduleName, const char * typeName )
{
	PyObject * ret = 0;
	ensurePythonInitialized();
	// Return a NEW reference
	SIP_BLOCK_THREADS
	PyObject * module = PyImport_ImportModule( (char*)moduleName );
	if( !module ) {
		LOG_1( "getPythonRecordClass: Unable to load " + QString::fromLatin1(moduleName) + " module" );
	} else {

		// Returns a BORROWED reference
		PyObject * moduleDict = PyModule_GetDict( module );
		Py_DECREF(module);
		if( !moduleDict ) {
			LOG_1( "getPythonRecordClass: Unable to get dict for " + QString::fromLatin1(moduleName) + " module" );
		} else
			// Returns a BORROWED reference
			ret = PyDict_GetItemString( moduleDict, typeName );
	}
	SIP_UNBLOCK_THREADS
	return ret;
}

// Returns a BORROWED reference
PyObject * getPythonRecordClass()
{
	return getPythonClass( "blur.Stone", "Record" );
}

PyObject * sipWrapRecord( Record * r, bool makeCopy, TableSchema * defaultType )
{
	PyObject * ret = 0;
	// First we convert to Record using sip methods
	static sipTypeDef * recordType = getRecordType( "blur.Stone", "Record" );
	sipTypeDef * type = recordType;
	if( type ) {
		if( makeCopy )
			ret = getSipAPI()->api_convert_from_new_type( new Record(*r), type, NULL );
		else {
			ret = getSipAPI()->api_convert_from_type( r, type, Py_None );
		}
	} else {
		LOG_1( "Stone.Record not found" );
		return 0;
	}

	Table * table = r->table();

	TableSchema * tableSchema = 0;
	
	if( table )
		tableSchema = table->schema();
	else if( defaultType )
		tableSchema = defaultType;
	else
		return ret;

	bool isErr = false;
	if( tableSchema ) {
		QString className = tableSchema->className();
	
		// Then we try to find the python class for the particular schema class type
		// from the desired module set using addSchemaCastModule
		// BORROWED ref
		PyObject * dict = getSchemaCastModule( tableSchema->schema() );
		if( dict ) {
			SIP_BLOCK_THREADS
			// BORROWED ref
			PyObject * klass = PyDict_GetItemString( dict, className.toLatin1().constData() );
			if( klass ) {
				PyObject * tuple = PyTuple_New(1);
				// Tuple now holds only ref to ret
				PyTuple_SET_ITEM( tuple, 0, ret );
				PyObject * result = PyObject_CallObject( klass, tuple );
				if( result ) {
					if( PyObject_IsInstance( result, klass ) == 1 ) {
						ret = result;
					} else {
						LOG_1( "Cast Ctor Result is not a subclass of " + className );
						Py_INCREF( ret );
						Py_DECREF( result );
					}
				} else {
					LOG_1( "runPythonFunction: Execution Failed, Error Was:\n" );
					PyErr_Print();
					isErr = true;
				}
				Py_DECREF( tuple );
			}
			SIP_UNBLOCK_THREADS
			if( isErr ) return 0;
		} else LOG_1( "No cast module set for schema" );
	} else LOG_1( "Table has no schema" );
	return ret;
}

PyObject * sipWrapRecordList( RecordList * rl, bool makeCopy, TableSchema * defaultType, bool allowUpcasting )
{
	PyObject * ret = 0;
	static sipTypeDef * recordType = getRecordType( "blur.Stone", "RecordList" );
	// First we convert to Record using sip methods
	sipTypeDef * type = recordType;
	if( type ) {
		if( makeCopy )
			ret = getSipAPI()->api_convert_from_new_type( new RecordList(*rl), type, NULL );
		else
			ret = getSipAPI()->api_convert_from_type( rl, type, Py_None );
	} else
		return 0;

	TableSchema * tableSchema = 0;
	if( allowUpcasting && !rl->isEmpty() ) { 
		Table * table = (*rl)[0].table();
		if( table ) tableSchema = table->schema();
		if( tableSchema ) {
			foreach( Record r, (*rl) ) {
				table = r.table();
				if( !table ) {
					tableSchema = 0;
					break;
				}
				TableSchema * ts = table->schema();
				if( ts != tableSchema && !tableSchema->isDescendant( ts ) ) {
					if( ts->isDescendant( tableSchema ) )
						tableSchema = ts;
					else {
						tableSchema = 0;
						break;
					}
				}
			}
		}
	}

	if( !tableSchema && defaultType )
		tableSchema = defaultType;

	if( !tableSchema ) return ret;

	QString className = tableSchema->className() + "List";
	
	// Then we try to find the python class for the particular schema class type
	// from the desired module set using addSchemaCastModule
	// BORROWED ref
	PyObject * dict = getSchemaCastModule( tableSchema->schema() );
	if( dict ) {
		bool isErr = false;
		SIP_BLOCK_THREADS
		// BORROWED ref
		PyObject * klass = PyDict_GetItemString( dict, className.toLatin1().constData() );
		if( klass ) {
			PyObject * tuple = PyTuple_New(1);
			PyTuple_SET_ITEM( tuple, 0, ret );
			PyObject * result = PyObject_CallObject( klass, tuple );
			if( result ) {
				if( PyObject_IsInstance( result, klass ) == 1 ) {
					// result is returned, the original ret is decref'ed in the tuple
					ret = result;
				} else {
					// Dont decref ret, it's owned by the tuple which is decref'ed below
					Py_DECREF( result );
				}
			} else{
				LOG_1( "runPythonFunction: Execution Failed, Error Was:\n" );
				PyErr_Print();
				isErr = true;
			}
			Py_DECREF( tuple );
		}
		SIP_UNBLOCK_THREADS
		if( isErr )
			return 0;
	} else LOG_1( "No cast module set for schema" );
	return ret;
}

RecordList recordListFromPyList( PyObject * a0 )
{
	SIP_SSIZE_T numItems = PyList_GET_SIZE(a0);
	RecordList ret;
	
	for (int i = 0; i < numItems; ++i)
	{
		ret += sipUnwrapRecord( PyList_GET_ITEM(a0,i) );
	}
	return ret;
}

PyObject * recordListGroupByCallable( const RecordList * rl, PyObject * callable, TableSchema * defaultType )
{
	PyObject *d = PyDict_New();

	if (!d)
		return NULL;
	
	bool error = false;
	static sipTypeDef * rl_type = getRecordType( "blur.Stone", "RecordList" );
	
	foreach( Record r, *rl ) {
		PyObject * py_r = sipWrapRecord( &r );
		
		// This should probably throw an exception
		if( !py_r )
			continue;
			
		// Tuple now holds only ref to py_r
		PyObject * tuple = PyTuple_New(1);
		PyTuple_SET_ITEM( tuple, 0, py_r );
		
		PyObject * key = PyObject_CallObject( callable, tuple );
		Py_DECREF(tuple);
		
		if( !key ) {
			error = true;
			break;
		}
		
		PyObject * entry = PyDict_GetItem( d, key );
		if( entry ) {
			if( getSipAPI()->api_can_convert_to_type(entry, rl_type, SIP_NOT_NONE | SIP_NO_CONVERTORS) ) {
				int isErr = 0;
				RecordList * rl = (RecordList*)getSipAPI()->api_convert_to_type( entry, rl_type, NULL, SIP_NOT_NONE | SIP_NO_CONVERTORS, 0, &isErr );
				if( !isErr )
					rl->append(r);
			}
		} else {
			RecordList rl(r);
			PyDict_SetItem( d, key, sipWrapRecordList( &rl, true, defaultType, /*allowUpcasting=*/ false ) );
		}
		Py_DECREF(key);
	}
	
	if( error ) {
		Py_DECREF(d);
		d = 0;
	}
	
	return d;
}

bool isPythonRecordInstance( PyObject * pyObject )
{
	bool ret = false;
	// Is this a class instance?
	SIP_BLOCK_THREADS
	ret = PyObject_IsInstance( pyObject, getPythonRecordClass() );
	SIP_UNBLOCK_THREADS
	return ret;
}

Record sipUnwrapRecord( PyObject * pyObject )
{
	static sipTypeDef * sipRecordType = getRecordType( "blur.Stone", "Record" );
	Record ret;
	SIP_BLOCK_THREADS
	if( getSipAPI()->api_can_convert_to_type( pyObject, sipRecordType, 0 ) ) {
		int state, err = 0;
		Record * r = (Record*)getSipAPI()->api_convert_to_type( pyObject, sipRecordType, 0, 0, &state, &err );
		if( !err )
			ret = *r;
		getSipAPI()->api_release_type( r, sipRecordType, state );
	}
	SIP_UNBLOCK_THREADS
	return ret;
	/*
	PyObject * recordClassObject = getPythonRecordClass();
	if( classObject && recordClassObject && sipRecordWrapper ) {
		SIP_BLOCK_THREADS
		if( PyClass_IsSubclass( classObject, recordClassObject ) ) {
			bool needRecordInstanceDeref = false;
			if( classObject != recordClassObject ) {
				PyObject * tuple = PyTuple_New(1);
				
				// tuple steals a ref
				Py_INCREF(pyObject);
				PyTuple_SetItem(tuple,0,pyObject);
				
				recordClassInstance = PyInstance_New( recordClassObject, tuple, 0 );
				Py_DECREF(tuple);
				needRecordInstanceDeref = true;
			} else
				recordClassInstance = pyObject;
	
			int err=0;
			if( recordClassInstance && getSipAPI()->api_can_convert_to_type( recordClassInstance, sipRecordWrapper, 0 ) ) {
				Record * r = (Record*)getSipAPI()->api_convert_to_type( recordClassInstance, sipRecordWrapper, 0, 0, 0, &err );
				ret = *r;
			}
			
			if( needRecordInstanceDeref && recordClassInstance )
				Py_DECREF( recordClassInstance );
		}
		SIP_UNBLOCK_THREADS
	}
	*/
}

QVariant unwrapQVariant( PyObject * pyObject )
{
	QVariant ret;
	if( !pyObject || pyObject == Py_None )
		return ret;

	int err = 0;
	const sipAPIDef * api = getSipAPI();
	// QString
	if( api->api_can_convert_to_type( pyObject, sipStringType(), 0 ) ) {
		QString * s = (QString*)api->api_convert_to_type( pyObject, sipStringType(), 0, 0, 0, &err );
		if( s ) ret = QVariant(*s);
	}

	// QDate
	else if( api->api_can_convert_to_type( pyObject, sipDateType(), 0 ) ) {
		QDate * d = (QDate*)api->api_convert_to_type( pyObject, sipDateType(), 0, 0, 0, &err );
		if ( d ) ret = QVariant(*d);
	}

	// QDateTime
	else if( api->api_can_convert_to_type( pyObject, sipDateTimeType(), 0 ) ) {
		QDateTime * d = (QDateTime*)api->api_convert_to_type( pyObject, sipDateTimeType(), 0, 0, 0, &err );
		if ( d ) ret = QVariant(*d);
	}

	// QTime
	else if( api->api_can_convert_to_type( pyObject, sipTimeType(), 0 ) ) {
		QTime * d = (QTime*)api->api_convert_to_type( pyObject, sipTimeType(), 0, 0, 0, &err );
		if ( d ) ret = QVariant(*d);
	}

	// QColor
	else if( api->api_can_convert_to_type( pyObject, sipColorType(), 0 ) ) {
		QColor * c = (QColor*)api->api_convert_to_type( pyObject, sipColorType(), 0, 0, 0, &err );
		if ( c ) ret = QVariant(*c);
	}

	// Interval
	else if( api->api_can_convert_to_type( pyObject, sipIntervalType(), 0 ) ) {
		Interval * i = (Interval*)api->api_convert_to_type( pyObject, sipIntervalType(), 0, 0, 0, &err );
		if( i ) ret = qVariantFromValue<Interval>(*i);
	}

	else if( api->api_can_convert_to_type( pyObject, sipImageType(), 0 ) ) {
		QImage * i = (QImage*)api->api_convert_to_type( pyObject, sipImageType(), 0, 0, 0, &err );
		if( i ) ret = QVariant(*i);
	}
	
	else {
		SIP_BLOCK_THREADS

		// Boolean
		if( PyBool_Check( pyObject ) )
			ret = QVariant( bool(PyObject_IsTrue( pyObject )) );

		// Int
		else if( PyInt_Check( pyObject ) )
			ret = QVariant( (int)PyInt_AsLong( pyObject ) );

		// Double
		else if( PyFloat_Check( pyObject ) )
			ret = QVariant( PyFloat_AsDouble( pyObject ) );

		else if( PyLong_Check( pyObject ) ) {
			{// Int
				int reti = PyLong_AsLong( pyObject );
				if( PyErr_Occurred() )
					PyErr_Clear();
				else
					ret = QVariant( reti );
			}
	
			{// Double
				double retd = PyLong_AsDouble( pyObject );
				if( PyErr_Occurred() )
					PyErr_Clear();
				else
					ret = QVariant( retd );
			}
	
			{// LongLong
				long long retll = PyLong_AsLongLong( pyObject );
				if( PyErr_Occurred() )
					PyErr_Clear();
				else
					ret = QVariant( retll );
			}
		} else if( isPythonRecordInstance( pyObject ) )
			ret = qVariantFromValue( sipUnwrapRecord( pyObject ) );
		else
			LOG_1( "Unable to unwrap python object:" + pyObjectRepr( pyObject ) );
		SIP_UNBLOCK_THREADS
	}
	return ret;
}

PyObject * wrapQVariant( const QVariant & var, bool stringAsPyString, bool noneOnFailure, int qvariantType )
{
	ensurePythonInitialized();
	PyObject * ret = 0;
	SIP_BLOCK_THREADS
	int qvt = var.userType();
	if( qvt == QVariant::Invalid )
		qvt = qvariantType;
	switch( qvt ) {
		case QVariant::Bool:
			ret = PyBool_FromLong( var.toBool() ? 1 : 0 );
			break;
		case QVariant::Color:
		{
			if( sipColorType() )
				ret = getSipAPI()->api_convert_from_new_type(new QColor(var.value<QColor>()), sipColorType(), 0);
			break;
		}
		case QVariant::Image:
		{
			if( sipImageType() )
				ret = getSipAPI()->api_convert_from_new_type(new QImage(var.value<QImage>()), sipImageType(), 0);
			break;
		}
		case QVariant::Date:
		{
			if( sipDateType() )
				ret = getSipAPI()->api_convert_from_new_type(new QDate(var.toDate()),sipDateType(),0);
			break;
		}
		case QVariant::DateTime:
		{
			if( sipDateTimeType() )
				ret = getSipAPI()->api_convert_from_new_type(new QDateTime(var.toDateTime()),sipDateTimeType(),0);
			break;
		}
		case QVariant::Double:
			ret = PyFloat_FromDouble( var.toDouble() );
			break;
		case QVariant::Int:
			ret = PyInt_FromLong(var.toInt());
			break;
		case QVariant::UInt:
			ret = PyInt_FromLong(var.toInt());
			break;
		case QVariant::LongLong:
			ret = PyLong_FromLongLong( var.toLongLong() );
			break;
		case QVariant::ULongLong:
			ret = PyLong_FromLongLong( var.toLongLong() );
			break;
		case QVariant::Time:
		{
			if( sipTimeType() )
				ret = getSipAPI()->api_convert_from_new_type(new QTime(var.toTime()),sipTimeType(),0);
			break;
		}
		case QVariant::String:
		{
			if( stringAsPyString ) {
				ret = PyString_FromString( var.toString().toLatin1().constData() );
			} else {
				if( sipStringType() )
					ret = getSipAPI()->api_convert_from_new_type(new QString(var.toString()),sipStringType(),0);
			}
			break;
		}
		case QVariant::StringList:
		{
			QStringList sl = var.toStringList();
			ret = PyList_New(0);
			foreach( QString s, sl )
				if( stringAsPyString || !sipStringType() )
					PyList_Append(ret, PyString_FromString( s.toLatin1().constData() ) );
				else
					PyList_Append(ret, getSipAPI()->api_convert_from_new_type(new QString(s),sipStringType(),0));
		}
		default:
			break;
	}
	if( qvt == qMetaTypeId<Record>() ) {
		Record r = qvariant_cast<Record>(var);
		ret = sipWrapRecord( &r );
	}
	if( qvt == qMetaTypeId<Interval>() )
		ret = getSipAPI()->api_convert_from_new_type(new Interval(qvariant_cast<Interval>(var)),sipIntervalType(),0);
	if( !ret && noneOnFailure ) {
		ret = Py_None;
		Py_INCREF(ret);
	}
	SIP_UNBLOCK_THREADS
	return ret;
}

QString pyObjectRepr( PyObject * pyObject )
{
	QString ret;
	SIP_BLOCK_THREADS
	PyObject * repr =  PyObject_Repr( pyObject );
	if( repr ) {
		ret = unwrapQVariant( repr ).toString();
		Py_DECREF( repr );
	}
	SIP_UNBLOCK_THREADS
	return ret;
}

PyObject * variantListToTuple( const VarList & args, int qvariantType )
{
	ensurePythonInitialized();
	PyObject * tuple = 0;
	SIP_BLOCK_THREADS
	tuple = PyTuple_New(args.size());
	int i=0;
	foreach( QVariant arg, args )
		PyTuple_SET_ITEM( tuple, i++, wrapQVariant(arg,false,true,qvariantType) );
	SIP_UNBLOCK_THREADS
	return tuple;
}

PyObject * variantListToPyList( const VarList & args, int qvariantType )
{
	ensurePythonInitialized();
	PyObject * list = 0;
	SIP_BLOCK_THREADS
	list = PyList_New(args.size());
	int i=0;
	foreach( QVariant arg, args )
		PyList_SET_ITEM( list, i++, wrapQVariant(arg,false,true,qvariantType) );
	SIP_UNBLOCK_THREADS
	return list;
}

QVariant runPythonFunction( PyObject * callable, PyObject * tuple )
{
	QVariant ret;
	SIP_BLOCK_THREADS
	if( !callable || !PyCallable_Check(callable) ) {
		LOG_1( "runPythonFunction: callable is not a valid PyCallable object" );
	} else {

		PyObject * result = PyObject_CallObject( callable, tuple );
		Py_DECREF( tuple );
	
		if( result ) {
			ret = unwrapQVariant( result );
			Py_DECREF( result );
		} else {
			LOG_1( "runPythonFunction: Execution Failed, Error Was:\n" );
			PyErr_Print();
		}
	}
	SIP_UNBLOCK_THREADS
	return ret;
}

QVariant runPythonFunction( const QString & moduleName, const QString & functionName, const VarList & varArgs )
{
	QVariant ret;
	ensurePythonInitialized();
	// Returns a NEW reference
	SIP_BLOCK_THREADS
	PyObject * module = PyImport_ImportModule( moduleName.toLatin1().data() );
	if( !module ) {
		LOG_1( "Unable to load python module: " + moduleName );
	} else {

		// Returns a BORROWED reference
		PyObject * moduleDict = PyModule_GetDict( module );
		Py_DECREF( module );
		if( !moduleDict ) {
			LOG_1( "Unable to retrieve dict for module: " + moduleName );
		} else {
		
			// Returns a BORROWED reference
			PyObject * function = PyDict_GetItemString( moduleDict, functionName.toLatin1().data() );
			if( !function ) {
				LOG_1( "Unable to find function: " + functionName + " inside module " + moduleName );
			} else {
				if( !PyCallable_Check( function ) ) {
					LOG_1( moduleName + "." + functionName + " is not a callable object" );
				} else {
				
					// Returns a NEW reference
					PyObject * args = variantListToTuple( varArgs );
					// Steals the new reference from args
					ret = runPythonFunction( function, args );
				}
			}
		}
	}
	SIP_UNBLOCK_THREADS
	return ret;
}

struct SchemaCastModule {
	Schema * schema;
	PyObject * dict;
};

static QList<SchemaCastModule> schemaCastModuleList;

void addSchemaCastNamedModule( Schema * schema, const QString & moduleName )
{
	// NEW ref
	SIP_BLOCK_THREADS
	PyObject * module = PyImport_ImportModule( moduleName.toLatin1().data() );
	if( module ) {
		// BORROWED ref
		PyObject * moduleDict = PyModule_GetDict( module );
		if( moduleDict ) {
			addSchemaCastTypeDict( schema, moduleDict );
		} else LOG_1( "Unable to get dict for module: " + moduleName );
		Py_DECREF( module );
	} else LOG_1( "Unable to load module: " + moduleName );
	SIP_UNBLOCK_THREADS
}

void addSchemaCastTypeDict( Schema * schema, PyObject * dict )
{
	removeSchemaCastModule(schema);
	SchemaCastModule scm;
	scm.schema = schema;
	scm.dict = dict;
	SIP_BLOCK_THREADS
	Py_INCREF(dict);
	SIP_UNBLOCK_THREADS
	schemaCastModuleList += scm;
}

void removeSchemaCastModule( Schema * schema )
{
	for( int i = 0; i < schemaCastModuleList.size(); i++ )
		if( schemaCastModuleList[i].schema == schema ) {
			SchemaCastModule & scm = schemaCastModuleList[i];
			SIP_BLOCK_THREADS
			Py_DECREF(scm.dict);
			SIP_UNBLOCK_THREADS
			schemaCastModuleList.removeAt(i);
		}
}

PyObject * getSchemaCastModule( Schema * schema )
{
	for( int i = 0; i < schemaCastModuleList.size(); i++ ) {
		SchemaCastModule & scm = schemaCastModuleList[i];
		if( scm.schema == schema )
			return scm.dict;
	}
	return 0;
}

void * pythonMetaTypeCtor( const void * copy )
{
	if( copy ) {
		SIP_BLOCK_THREADS
		Py_INCREF( (PyObject*)copy );
		SIP_UNBLOCK_THREADS
		return const_cast<void*>(copy);
	}
	return 0;
}

void pythonMetaTypeDtor( void * pyObject )
{
	SIP_BLOCK_THREADS
	Py_DECREF((PyObject*)pyObject);
	SIP_UNBLOCK_THREADS
}

int registerPythonQMetaType( PyObject * type )
{
	int ret = 0;
	PyObject * pyname = PyObject_GetAttrString( type, "__name__" );
	if( !pyname ) {
		printf( "registerPythonQMetaType: Unabled to get attribute __name__\n" );
		return ret;
	}
	const char * typeName = PyString_AsString(pyname);
	if( typeName ) {
		ret = QMetaType::registerType( typeName, reinterpret_cast<QMetaType::Destructor>(pythonMetaTypeDtor), reinterpret_cast<QMetaType::Constructor>(pythonMetaTypeCtor) );
		// Dont return success when it's returning an already registered builtin type
		if( ret < QMetaType::User ) ret = 0;
	} else
		printf("registerPythonQMetaType: __name__ attribute is not a string\n");
	Py_DECREF(pyname);
	return ret;
}

int findQVariantType( PyObject * type )
{
	int ret = 0;
	PyObject * pyname = PyObject_GetAttrString( type, "__name__" );
	if( !pyname ) {
		printf( "findQVariantType: Unabled to get attribute __name__\n" );
		return ret;
	}
	const char * typeName = PyString_AsString(pyname);
	if( typeName ) {
		ret = QMetaType::type(typeName);
	} else
		printf("findQVariantType: __name__ attribute is not a string\n");
	Py_DECREF(pyname);
	return ret;
}

PyObject * qvariantFromPyObject( PyObject * object )
{
	PyObject * klass = PyObject_GetAttrString( object, "__class__" );
	if( klass ) {
 		int type = findQVariantType( klass );
		if( type >= QMetaType::User ) {
			QVariant * ret = new QVariant( type, (void*)object );
			return getSipAPI()->api_convert_from_new_type(ret,sipQVariantType(),0);
		} else
			printf("qvariantFromPyObject: QMetaType::Type %i is not valid\n", type);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject * pyObjectFromQVariant( PyObject * py_qvariant )
{
	if( getSipAPI()->api_can_convert_to_type( py_qvariant, sipQVariantType(), 0 ) ) {
		int err = 0;
		QVariant * v = (QVariant*)getSipAPI()->api_convert_to_type( py_qvariant, sipQVariantType(), 0, 0, 0, &err );
		if( v ) {
			PyObject * object = (PyObject*)v->data();
			Py_INCREF(object);
			return object;
		} else
			printf("pyObjectFromQVariant: Unable to convert PyObject * to QVariant with sipConvertToInstance\n");
	}
	Py_INCREF(Py_None);
	return Py_None;
}

QString pythonExceptionTraceback( bool clearException )
{
	/*
		import traceback
		return '\n'.join(traceback.format_exc())
	*/
	QString ret;
	bool success = false;
	SIP_BLOCK_THREADS

	PyObject * type, * value, * traceback;
	/* Save the current exception */
	PyErr_Fetch(&type, &value, &traceback);
	if( type ) {
	
		PyObject * traceback_module = PyImport_ImportModule("traceback");
		if (traceback_module) {

			/* Call the traceback module's format_exception function, which returns a list */
			PyObject * traceback_list = PyObject_CallMethod(traceback_module, "format_exception", "OOO", type, value ? value : Py_None, traceback ? traceback : Py_None);
			if( traceback_list ) {

				PyObject * separator = PyString_FromString("");
				if( separator ) {

					PyObject * retString = PyUnicode_Join(separator, traceback_list);
					if( retString ) {
						ret = unwrapQVariant(retString).toString();
						success = true;
						Py_DECREF(retString);

					} else
						ret = "PyUnicode_Join failed";

					Py_DECREF(separator);
				} else
					ret = "PyUnicode_FromString failed";

				Py_DECREF(traceback_list);
			} else
				ret = "Failure calling traceback.format_exception";

			Py_DECREF(traceback_module);
		} else
			ret = "Unable to load the traceback module, can't get exception text";
	} else
		ret = "pythonExceptionTraceback called, but no exception set";

	if( clearException ) {
		Py_DECREF(type);
		Py_XDECREF(value);
		Py_XDECREF(traceback);
	} else
		PyErr_Restore(type,value,traceback);

	SIP_UNBLOCK_THREADS

	// Ret is an error message if success is false
	if( !success )
		LOG_1( ret );

	return ret;
}

void printPythonStackTrace()
{
	SIP_BLOCK_THREADS
	PyObject * traceback_module = PyImport_ImportModule("traceback");
	if (traceback_module) {

		/* Call the traceback module's format_exception function, which returns a list */
		PyObject * ret = PyObject_CallMethod(traceback_module, "print_stack","");
		if( !ret ) {
			LOG_1( pythonExceptionTraceback(true) );
		}
		Py_XDECREF(ret);
	} else
		LOG_1( "Unable to load the traceback module, can't get exception text" );
	SIP_UNBLOCK_THREADS
}

/*
	Checks to see if we own the GIL
*/
int PyGILState_Check2(void)
{
	PyThreadState * tstate = _PyThreadState_Current;
	return tstate && (tstate == PyGILState_GetThisThreadState());
}