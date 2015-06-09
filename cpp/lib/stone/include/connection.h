
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdexcept>

#include <qdatetime.h>
#include <qmap.h>
#include <qsqldatabase.h>
#include <qvariant.h>
#include <qvector.h>

#include "record.h"


namespace Stone {
class JoinedSelect;
class Schema;
class TableSchema;
class IndexSchema;
class Table;
class Expression;

struct RecordReturn
{
	// The table if no tableOidPos exists
	Table * table;
	// Holds the column that contains the return value's table oid for this record, -1 if none
	int tableOidPos;
	// For each table, a vector of integers that represent the column of the result set that contains
	// each column of data in the table.  -1 for columns that weren't selected.
	// The size of the vector must match table->schema()->fieldCount().
	QMap<Table*,QVector<int> > columnPositions;
};

/// Thrown any time a database error happens except connection
/// errors which are handled with the LostConnectionException class.
class STONE_EXPORT SqlException : public std::exception
{
public:
	SqlException( const QString & sql, const QString & error );
	~SqlException() throw() {}
	QString sql() const;
	QString error() const;
	virtual const char * what() const throw();
protected:
	QString mSql, mError;
	mutable QByteArray mCString;
};

/// This is thrown if there is an error executing sql and
/// either a transaction is open or the connection retries
/// expire.  The reason it never reconnects when inside a
/// transaction is that it would introduce non atomic behavior
/// for code that needs to run in a transaction if a statement
/// was automatically executed outside the transaction in a new
/// connection.
/// see Connection::setMaxConnectionAttempts
class STONE_EXPORT LostConnectionException : public std::exception
{
public:
	LostConnectionException();
	~LostConnectionException() throw() {}
	virtual const char * what() const throw();
};

/**
* \class Connection
*
* Connection handles communication to the database server.
* Only Postgresql is supported.
*
* \ingroup Stone
*/
class STONE_EXPORT Connection : public QObject
{
Q_OBJECT
public:
	Connection(QObject * parent=0);
	virtual ~Connection() {}

	static Connection * create( const QString & dbType );
	static Connection * createFromIni( IniConfig & , const QString & section );

	/// Reads all options from the ini object
	virtual void setOptionsFromIni( const IniConfig & );

	QString type() const { return mDatabaseType; }

	/// hostname
	QString host() const { return mHost; }
	void setHost( const QString & host );

	/// port number
	int port() const { return mPort; }
	void setPort( int port );

	/// database instance name
	QString databaseName() const { return mDatabaseName; }
	void setDatabaseName( const QString & );

	/// database user to connect as
	QString userName() const { return mUserName; }
	void setUserName( const QString & userName );

	/// database user password
	QString password() const { return mPassword; }
	void setPassword( const QString & password );

	/// In seconds
	int reconnectDelay() const { return mReconnectDelay; }
	void setReconnectDelay( int reconnectDelay );

	/// This is the maximum number of tries to connect to the database when executing a query.
	/// Calling checkConnection or reconnect will not check this value.
	/// If set to 0 there will be no automatic (re)connects, but the database can still
	/// be used after manually initiating the connection with checkConnection or reconnect.
	/// If set to -1 there is no limit.  The number of tries will be reset after each successful connection.
	int maxConnectionAttempts() const { return mMaxConnectionAttempts; }
	void setMaxConnectionAttempts( int maxConnectionAttempts );
	
	enum Capabilities {
		Cap_Inheritance = 		1 << 0,
		Cap_MultipleInsert = 	1 << 1,
		Cap_Returning = 		1 << 2,
		Cap_Transactions = 		1 << 3,
		Cap_CheckPoints = 		1 << 4,
		Cap_IndexCreation = 	1 << 5,
		Cap_MultiTableSelect = 	1 << 6,
		Cap_TableOids = 		1 << 7,
		Cap_Notifications = 	1 << 8,
		Cap_ChangeNotifications = 1 << 9
	};

	/** Returns an enum containing each of the Capabilities of this
	  * of this connection OR'ed together. */
	virtual Capabilities capabilities() const = 0;

	virtual QString lastErrorText() const { return mLastErrorText; }

	/** Reconnects to the database.  If currently connected then the connection will
	 *  be closed. If reconnect fails, it will set the errorText to a description of why.
	 */
	virtual bool reconnect() = 0;
	
	/** Returns true if there is currently a connection to the database. */
	virtual bool isConnected() = 0;

	/** Checks if the database is connected, if not it will call
	 *  reconnect.
	 */
	virtual bool checkConnection();

	/** Returns a string with the host/port/user/password and options, useful for error messages */
	virtual QString connectString() = 0;

	virtual bool closeConnection() = 0;
	
	//
	// Db table verification and creation
	//
	virtual bool tableExists( TableSchema * schema ) = 0;
	/// Verify that the table and needed columns exist in the database
	virtual bool verifyTable( TableSchema * schema, bool createMissingColumns = false, QString * output=0 ) = 0;
	/// Trys to create the table in the database
	virtual bool createTable( TableSchema * schema, QString * output = 0 ) = 0;

	virtual TableSchema * importTableSchema() = 0;

	virtual Schema * importDatabaseSchema() = 0;

	virtual uint newPrimaryKey( TableSchema * ) = 0;

	/** Executes the query using vars as bound values
	 *  Returns the executed query object.
	 *  Use next or isActive to query whether the sql
	 *  was executed correctly.
	 *  Uses exec bool exec( QSqlQuery &, reExecLostConn ) internally
	 *  and the documentation for that function applies to this.
	 */
	virtual QSqlQuery exec( const QString & sql, const QList<QVariant> & vars = QList<QVariant>(), bool reExecLostConn = true, Table * table = 0 ) = 0;

	/** Executes query.
	 *  Automatically calls Database::lockDb and Database::unlockDb.
	 *  Automatically reconnects and retries the query if the connection
	 *  was lost while the query was executing if reExecLostConn=true.
	 *  This function automatically logs any errors and
	 *  passes them to SqlErrorHandler::instance()->handleError.
	 */
	virtual bool exec( QSqlQuery & query, bool reExecLostConn = true, Table * table = 0 ) = 0;

	virtual RecordList executeExpression( Table * table, FieldList fields, const Expression & exp );
	virtual QMap<Table*,RecordList> executeExpression( Table * table, const RecordReturn & rr, const Expression & exp );

	/// Selects all columns except those marked NoDefaultSelect, using from and args
	/// It is expected that from will return rows of type table
	virtual RecordList selectFrom( Table * table, const QString & from, const QList<QVariant> & args = QList<QVariant>() ) = 0;

	/// Selects all columns except those marked NoDefaultSelect, from table using where and args
	virtual RecordList selectOnly( Table *, const QString & where = QString::null, const QList<QVariant> & vars = QList<QVariant>() ) = 0;
	virtual QList<RecordList> joinedSelect( const JoinedSelect &, QString where, QList<QVariant> vars ) = 0;
	
	/// Selects from multiple tables using UNION and returns results per table
	/// Only supported if the connection returns Cap_MultiTableSelect
	virtual QMap<Table *, RecordList> selectMulti( QList<Table*>,
		const QString & /*innerWhere*/ = QString::null, const QList<QVariant> & /*innerArgs*/ = QList<QVariant>(),
		const QString & /*outerWhere*/ = QString::null, const QList<QVariant> & /*outerArgs*/ = QList<QVariant>() )
	{ return QMap<Table*,RecordList>(); }

	virtual void selectFields( Table * table, RecordList, FieldList ) = 0;
	
	/// Inserts a RecordList into the database, by default use the sequence to 
	/// generate a new primary key.
	///
	/// Implementations must set the primary key on each record
	virtual void insert( Table *, const RecordList & rl ) = 0;

	/**
	 * Generates and executes a sql update
	 * to the database.
	 **/
	virtual bool update( Table *, RecordImp * imp, Record * returnValues = 0 ) = 0;
	virtual bool update( Table * table, RecordList records, RecordList * returnValues = 0 ) = 0;

	/// deletes a list of primary key ids from a table
	virtual int remove( Table *, const QString & keyList, QList<int> * rowsDeleted = 0 ) = 0;

	/// Implement if providing Cap_Transactions
	virtual bool beginTransaction() { return false; }
	virtual bool commitTransaction() { return false; }
	virtual bool rollbackTransaction() { return false; }

	/// Implement if providing Cap_IndexCreation
	virtual bool createIndex( IndexSchema * ) { return false; }

	virtual void listen( const QString & /*notificationName*/ ) {}
	
	virtual bool verifyChangeTrigger( TableSchema *, bool /* create */ = false ) { return false; }
	
	virtual TableSchema * tableByOid( uint oid, Schema * );
	virtual uint oidByTable( TableSchema * );

	// Use lowercase names
	QStringList pythonStackTraceOnTables() const;
	void setPythonStackTraceOnTables( const QStringList & tables );
	void addPythonStackTraceOnTable( const QString & tableName );
	void removePythonStackTraceOnTable( const QString & tableName );

signals:
	/** Emmitted when the connection is lost
	 * control will not return to the main event
	 * loop until the connection is regained
	 */
	void connectionLost();

	/** Called immediatly after connectionLost
	 *  Used for the connection dialog, it will
	 *  Not return until the connection is
	 *  regained, or until QApplication::exit/quit
	 *  is called
	 */
	void connectionLost2();

	/** Emitted whenever a connection is made */
	void connected();

protected:
	QString mLastErrorText;
	QString mHost, mUserName, mPassword, mDatabaseName;
	int mPort, mReconnectDelay, mConnectionAttempts, mMaxConnectionAttempts;
	QString mDatabaseType;
	// List of tables to throw an exception whenever a sql command is to be executed, for debugging purposes
	QStringList mPythonStackTraceOnTables;
};

/** 
* \class QSqlDbConnection
*
* Subclass of connection that takes care of the generic QSql database connection stuff
*
* \ingroup Stone
*/
class STONE_EXPORT QSqlDbConnection : public Connection
{
public:
	QSqlDbConnection( const QString & driverName );
	~QSqlDbConnection();

	/// Reads all options from the ini object
	virtual void setOptionsFromIni( const IniConfig & );

	virtual Capabilities capabilities() const;

	/** Returns the QSqlDatabase object used for the database connection */
	QSqlDatabase db() const { return mDb; }

	/** Executes the query using vars as bound values
	 *  Returns the executed query object.
	 *  Use next or isActive to query whether the sql
	 *  was executed correctly.
	 *  Uses exec bool exec( QSqlQuery &, reExecLostConn ) internally
	 *  and the documentation for that function applies to this.
	 */
	virtual QSqlQuery exec( const QString & sql, const QList<QVariant> & vars = QList<QVariant>(), bool reExecLostConn = true, Table * table = 0 );

	/** Executes query.
	 *  Automatically calls Database::lockDb and Database::unlockDb.
	 *  Automatically reconnects and retries the query if the connection
	 *  was lost while the query was executing if reExecLostConn=true.
	 *  This function automatically logs any errors and
	 *  passes them to SqlErrorHandler::instance()->handleError.
	 */
	virtual bool exec( QSqlQuery & query, bool reExecLostConn = true, Table * table = 0 );
	QSqlQuery fakePrepare( const QString & sql );
	bool exec( QSqlQuery & query, bool reExecLostConn, Table * table, bool usingFakePrepareHack );

	virtual bool beginTransaction();
	virtual bool commitTransaction();
	virtual bool rollbackTransaction();

	/** Reconnects to the database.  If currently connected then the connection will
	 *  be closed. If reconnect fails, it will set the errorText to a description of why.
	 */
	virtual bool reconnect();
	
	/** Returns true if there is currently a connection to the db */
	virtual bool isConnected();

	virtual bool closeConnection();
	
	/** Returns a string with the host/port/user/password and options, useful for error messages */
	virtual QString connectString();

	virtual void listen( const QString & notificationName );

	static bool isConnectionError( const QSqlError & e );
protected:
	void setupSqlDbOptions();

	QSqlDatabase mDb;
	QString mExtraConnectOptions;
	bool mInsideTransaction;
};

} // namespace

#endif // CONNECTION_H

