
// QList<RecordList> is implemented as a Python list of RecordLists.
%MappedType QList<RecordList>
{
%TypeHeaderCode
#include <qlist.h>
%End

%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->size())) == NULL)
        return NULL;

    // Set the list elements.
    for (int i = 0; i < sipCpp->size(); ++i)
    {
		RecordList rl = sipCpp->value(i);

		PyList_SET_ITEM(l, i, sipConvertFromNewType( new RecordList(rl), sipType_RecordList, NULL ) );
    }

    return l;
%End

%ConvertToTypeCode
    SIP_SSIZE_T len;

    // Check the type if that is all that is required.
	if (sipIsErr == NULL)
	{
		if (!PySequence_Check(sipPy) || (len = PySequence_Size(sipPy)) < 0)
			return 0;

		for (SIP_SSIZE_T i = 0; i < len; ++i)
		{
			PyObject *itm = PySequence_ITEM(sipPy, i);
			bool ok = (itm && sipCanConvertToType(itm, sipType_RecordList, SIP_NOT_NONE));

			Py_XDECREF(itm);

			if (!ok)
				return 0;
		}

		return 1;
	}

	QList<RecordList> *ql = new QList<RecordList>;
	len = PySequence_Size(sipPy);

	for (SIP_SSIZE_T i = 0; i < len; ++i)
	{
		PyObject *itm = PySequence_ITEM(sipPy, i);
		int state;
		RecordList * t = reinterpret_cast<RecordList*>(sipConvertToType(itm, sipType_RecordList, sipTransferObj, SIP_NOT_NONE, &state, sipIsErr));

		Py_DECREF(itm);

		if (*sipIsErr)
		{
			sipReleaseType(t, sipType_RecordList, state);

			delete ql;
			return 0;
		}

		ql->append(*t);

		sipReleaseType(t, sipType_RecordList, state);
	}

	*sipCppPtr = ql;

	return sipGetState(sipTransferObj);
%End
};


class exception {
%TypeHeaderCode
#include <stdexcept>
using std::exception;
%End
public:
	exception () throw();
	exception (const exception&) throw();
	//exception& operator= (const exception&) throw();
	virtual ~exception() throw();
	virtual const char* what() const throw();
	const char * __str__();
%MethodCode
	sipRes = sipCpp->what();
%End
	const char * __repr__();
%MethodCode
	sipRes = sipCpp->what();
%End


};

class SqlException : exception
{
%TypeHeaderCode
#include <connection.h>
using std::exception;
%End
public:
	SqlException( const QString & sql, const QString & error );
	~SqlException() throw();
	QString sql() const;
	QString error() const;
	virtual const char * what() const throw();
};

class LostConnectionException : public exception
{
%TypeHeaderCode
#include <connection.h>
using std::exception;
%End
public:
	LostConnectionException();
	~LostConnectionException() throw();
	virtual const char * what() const throw();
};

/*
%Exception SqlException(SIP_Exception)
{
%TypeHeaderCode
#include <connection.h>
%End
%RaiseCode
	const char *detail = sipExceptionRef.what();
	SIP_BLOCK_THREADS
	PyErr_SetString( sipException_SqlException, detail );
	SIP_UNBLOCK_THREADS
%End
};
*/

class Connection : QObject
{
%TypeHeaderCode
#include <connection.h>
#include <pgconnection.h>
%End
public:

%ConvertToSubClassCode
	sipClass = NULL;
	if( dynamic_cast<PGConnection*>(sipCpp) )
		sipClass = sipClass_PGConnection;
%End

	virtual ~Connection();

	static Connection * create( const QString & dbType );
	static Connection * createFromIni( IniConfig & , const QString & section );

	// Reads all options from the ini object
	virtual void setOptionsFromIni( const IniConfig & );

	QString type() const;

	QString host() const;
	int port() const;
	QString databaseName() const;
	QString userName() const;
	QString password() const;

	void setHost( const QString & host );
	void setPort( int port );
	void setDatabaseName( const QString & );
	void setUserName( const QString & userName );
	void setPassword( const QString & password );

	int reconnectDelay() const;
	void setReconnectDelay( int reconnectDelay );

	int maxConnectionAttempts() const;
	void setMaxConnectionAttempts( int maxConnectionAttempts );

	enum Capabilities {
		Cap_Inheritance = 		1,
		Cap_MultipleInsert = 	2,
		Cap_Returning = 		4,
		Cap_Transactions = 		8,
		Cap_CheckPoints = 		16,
		Cap_IndexCreation = 	32
	};

	virtual Connection::Capabilities capabilities() const = 0;

	virtual QString lastErrorText() const;

	virtual bool reconnect() = 0;
	
	virtual bool isConnected() = 0;

	virtual bool checkConnection();

	virtual bool closeConnection() = 0;

	virtual QString connectString() = 0;

	virtual bool tableExists( TableSchema * schema ) throw(SqlException,LostConnectionException) = 0;

	virtual bool verifyTable( TableSchema * schema, bool createMissingColumns = false, QString * output=0 ) throw(SqlException,LostConnectionException) = 0;

	virtual bool createTable( TableSchema * schema, QString * output = 0 ) throw(SqlException,LostConnectionException) = 0;

	virtual TableSchema * importTableSchema() throw(SqlException,LostConnectionException) = 0;

	virtual Schema * importDatabaseSchema() throw(SqlException,LostConnectionException) = 0;

	virtual uint newPrimaryKey( TableSchema * ) throw(SqlException,LostConnectionException) = 0;

	virtual QSqlQuery exec( const QString & sql, const QList<QVariant> & vars = QList<QVariant>(), bool reExecLostConn = true, Table * table = 0 ) throw(SqlException,LostConnectionException) = 0 /PyName=exec_/;

	virtual bool exec( QSqlQuery & query, bool reExecLostConn = true, Table * table = 0 ) throw(SqlException,LostConnectionException) = 0 /PyName=exec_/;

	virtual RecordList executeExpression( Table * table, QList<Field*> fields, const Expression & exp ) throw(SqlException,LostConnectionException);
	//virtual QMap<Table*,RecordList> executeExpression( Table * table, const RecordReturn & rr, const Expression & exp );

	virtual RecordList selectFrom( Table * table, const QString & from, const QList<QVariant> & args ) throw(SqlException,LostConnectionException) = 0;

	virtual RecordList selectOnly( Table *, const QString & where = QString::null, const QList<QVariant> & vars = QList<QVariant>() ) throw(SqlException,LostConnectionException) = 0;
	virtual QList<RecordList> joinedSelect( const JoinedSelect &, QString where, QList<QVariant> vars ) throw(SqlException,LostConnectionException) = 0;

	virtual void selectFields( Table * table, RecordList, QList<Field*> ) throw(SqlException,LostConnectionException) = 0;

	virtual void insert( Table *, const RecordList & rl ) throw(SqlException,LostConnectionException) = 0;

	virtual bool update( Table *, RecordImp * imp, Record * returnValues = 0 ) throw(SqlException,LostConnectionException) = 0;
	virtual bool update( Table * table, RecordList records, RecordList * returnValues = 0 ) throw(SqlException,LostConnectionException) = 0;

	virtual int remove( Table *, const QString &, QList<int> * rowsDeleted = 0 ) throw(SqlException,LostConnectionException) = 0;

	virtual bool beginTransaction() throw(SqlException,LostConnectionException);
	virtual bool commitTransaction() throw(SqlException,LostConnectionException);
	virtual bool rollbackTransaction() throw(SqlException,LostConnectionException);

	virtual bool createIndex( IndexSchema * ) throw(SqlException,LostConnectionException);

	QStringList pythonStackTraceOnTables() const;
	void setPythonStackTraceOnTables( const QStringList & tables );
	void addPythonStackTraceOnTable( const QString & tableName );
	void removePythonStackTraceOnTable( const QString & tableName );

signals:
	void connectionLost();

	void connectionLost2();

	void connected();
};

class QSqlDbConnection : Connection /Abstract/
{
%TypeHeaderCode
#include <connection.h>
%End

public:
	QSqlDbConnection( const QString & driverName );
	~QSqlDbConnection();

	virtual void setOptionsFromIni( const IniConfig & );

	virtual Connection::Capabilities capabilities() const;

	QSqlDatabase db() const;

	virtual QSqlQuery exec( const QString & sql, const QList<QVariant> & vars = QList<QVariant>(), bool reExecLostConn = true, Table * table = 0 ) throw(SqlException,LostConnectionException) /PyName=exec_/;

	virtual bool exec( QSqlQuery & query, bool reExecLostConn = true, Table * table = 0 ) throw(SqlException,LostConnectionException) /PyName=exec_/;

	virtual bool beginTransaction() throw(SqlException,LostConnectionException);
	virtual bool commitTransaction() throw(SqlException,LostConnectionException);
	virtual bool rollbackTransaction() throw(SqlException,LostConnectionException);

	virtual bool reconnect();
	
	virtual bool isConnected();

	virtual bool checkConnection();

	virtual bool closeConnection();

	virtual QString connectString();

	static bool isConnectionError( const QSqlError & e );
};
