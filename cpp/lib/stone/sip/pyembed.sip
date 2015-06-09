
SIP_PYOBJECT wrapQVariant( const QVariant & var, bool stringAsPyString = false, bool noneOnFailure = true, int qvariantType = 0 ) /HoldGIL/;

QVariant unwrapQVariant( SIP_PYOBJECT pyObject ) /HoldGIL/;

SIP_PYOBJECT variantListToTuple( const QList<QVariant> & args, int qvariantType = 0  ) /HoldGIL/;

SIP_PYOBJECT variantListToPyList( const QList<QVariant> & args, int qvariantType = 0  ) /HoldGIL/;

SIP_PYCALLABLE getCompiledFunction( const char * functionName, Table * table, int pkey, const QString & pCode, const QString & codeName = QString() ) /HoldGIL/;

SIP_PYCALLABLE getCompiledFunction( const char * functionName, const Record & r, const QString & pCode, const QString & codeName = QString() ) /HoldGIL/;

void addSchemaCastTypeDict( Schema * schema, SIP_PYOBJECT dict ) /HoldGIL/;

void removeSchemaCastModule( Schema * schema );

SIP_PYOBJECT getSchemaCastModule( Schema * schema );

bool isPythonRecordInstance( SIP_PYOBJECT pyObject );

Record sipUnwrapRecord( SIP_PYOBJECT pyObject );

SIP_PYOBJECT sipWrapRecord( Record * ) /HoldGIL/;

SIP_PYOBJECT sipWrapRecordList( RecordList * ) /HoldGIL/;

int registerPythonQMetaType( SIP_PYOBJECT type ) /HoldGIL/;
SIP_PYOBJECT qvariantFromPyObject( SIP_PYOBJECT object ) /HoldGIL/;
SIP_PYOBJECT pyObjectFromQVariant( SIP_PYOBJECT py_qvariant ) /HoldGIL/;
