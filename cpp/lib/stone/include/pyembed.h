
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

#ifndef PY_EMBED_H
#define PY_EMBED_H

namespace Stone {
class Table;
}
using namespace Stone;

extern "C" {
	#include <Python.h>
	#include <sip.h>
//	#include <node.h>
//	#include <compile.h>
//	#include <import.h>
//	#include <marshal.h>
//	#include <graminit.h>
//	#undef expr
//	#include <eval.h>
};

#include <qstring.h>

#include "blurqt.h"
#include "table.h"

class STONE_EXPORT PythonException : public std::exception
{
public:
	PythonException();
	virtual ~PythonException() throw();
	void restore();
protected:
	PyObject *type, *value, *traceback;
};

/// Returns a pointer to the single sip api object.  This is looked up via
/// sip's python module dictionary, and cached in a static variable for the
/// life of the program.
STONE_EXPORT const sipAPIDef * getSipAPI();

/// Returns the sip module by name, does not attempt to load the module
//STONE_EXPORT sipExportedModuleDef * getSipModule( const char * name );

/// Returns the exported type inside a given sip module
/// This lookup is not cached and the time to lookup is O(n), where n is the number
/// of types in the module.
STONE_EXPORT sipTypeDef * getSipType( const char * module_name, const char * typeName );

/// Converts a QVariant to a PyObject *.  If this function fails it will return 0
/// If it succeeds it will return a NEW reference.
///
/// Supports Double, Int, UInt, LongLong, Time, Date, DateTime, String, StringList
/// If qvariant type is not 0, and the variant is null and has no type, then a 0/empty
/// python value of the type is returned.  For example, if qvariantType == QVariant::String, then '' is returned
/// if qvariantType == QVariant::Double, then 0.0 is returned, instead of returning None.
STONE_EXPORT PyObject * wrapQVariant( const QVariant & var, bool stringAsPyString = false, bool noneOnFailure = true, int qvariantType = 0 );

/// Converts a PyObject * to a QVariant.
/// Supports Double, Int, UInt, LongLong, Time, Date, DateTime, String, StringList, Record(and subclasses)
STONE_EXPORT QVariant unwrapQVariant( PyObject * pyObject );

/// Returns the sipWrapperType * to the Record subclass for className, inside module
STONE_EXPORT sipTypeDef * getRecordType( const char * module, const QString & className );

/// Wraps the record object using the sip wrapper for Record, or for the Record subclass
/// that corrosponds to record.table()->className().
/// Returns a NEW reference if successfull, otherwise 0
STONE_EXPORT PyObject * sipWrapRecord( Record *, bool makeCopy = true, TableSchema * defaultType = 0 );

/// Wraps the recordlist object using the sip wrapper for RecordList, or for the RecordList subclass
/// that corrosponds to records' table()->className().
/// Returns a NEW reference if successfull, otherwise 0
/// If allowUpcasting=true, then the list will be checked and the returned type will be the
/// highest common anscestor type.  For example a ProjectList will be returned even if defaultType is Element.
/// If allowUpcasting=false then defaultType will always be used, or plain RecordList if defaultType is 0.
STONE_EXPORT PyObject * sipWrapRecordList( RecordList *, bool makeCopy = true, TableSchema * defaultType = 0, bool allowUpcasting = true );

STONE_EXPORT RecordList recordListFromPyList( PyObject * pyObject );

/// Calls callable on each record and uses the return value as the key for dict insertion
/// Each dict value is a RecordList class or subclass if defaultType is passed
STONE_EXPORT PyObject * recordListGroupByCallable( const RecordList * rl, PyObject * callable, TableSchema * defaultType = 0 );

/// Returns true if the python object is a blur.Stone.Record class instance or subclass instance
STONE_EXPORT bool isPythonRecordInstance( PyObject * pyObject );

/// Returns the record wrapped by the pyObject, it must fullfill isPythonRecordInstance
/// If the conversion fails, and Invalid Record is returned.
STONE_EXPORT Record sipUnwrapRecord( PyObject * pyObject );

/// Returns a QString result of the repr() function of the object, usefull for debugging the objects
STONE_EXPORT QString pyObjectRepr( PyObject * pyObject );

/// Returns a NEW reference to a PyTuple object filled with args, converted using wrapQVariant
/// Check wrapQVariant docs for qvariantType explanation
STONE_EXPORT PyObject * variantListToTuple( const VarList & args, int qvariantType = 0 );

/// Returns a NEW reference to a PyList object filled with args, converted using wrapQVariant
/// Check wrapQVariant docs for qvariantType explanation
STONE_EXPORT PyObject * variantListToPyList( const VarList & args, int qvariantType = 0 );

/// STEALS a reference from 'tuple', if the function is callable
STONE_EXPORT QVariant runPythonFunction( PyObject * callable, PyObject * tuple );

STONE_EXPORT QVariant runPythonFunction( const QString & module, const QString & function, const VarList & args );

typedef struct 
{
	PyCodeObject * code;
	PyObject * locals;
	QString codeString;
} CompiledPythonFragment;

STONE_EXPORT CompiledPythonFragment * getCompiledCode( Table * table, int pkey, const QString & pCode, const QString & codeName = QString() );
STONE_EXPORT CompiledPythonFragment * getCompiledCode( const Record & r, const QString & pCode, const QString & codeName = QString() );

/// Returns a BORROWED reference
STONE_EXPORT PyObject * getCompiledFunction( const char * functionName, Table * table, int pkey, const QString & pCode, const QString & codeName = QString() );

/// Returns a BORROWED reference
STONE_EXPORT PyObject * getCompiledFunction( const char * functionName, const Record & r, const QString & pCode, const QString & codeName = QString() );

STONE_EXPORT void addSchemaCastNamedModule( Schema * schema, const QString & moduleName );

STONE_EXPORT void addSchemaCastTypeDict( Schema * schema, PyObject * dict );

STONE_EXPORT void removeSchemaCastModule( Schema * schema );

STONE_EXPORT PyObject * getSchemaCastModule( Schema * schema );

STONE_EXPORT int registerPythonQMetaType( PyObject * type );
STONE_EXPORT PyObject * qvariantFromPyObject( PyObject * object );
STONE_EXPORT PyObject * pyObjectFromQVariant( PyObject * py_qvariant );

/// Returns the result of traceback.format_exception, converted into a QString
/// Clears the python exception by default
STONE_EXPORT QString pythonExceptionTraceback( bool clearException = true );

STONE_EXPORT void printPythonStackTrace();

#endif // PY_EMBED_H

