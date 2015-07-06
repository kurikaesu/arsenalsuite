
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

#include "Python.h"

#include <qdatetime.h>
#include <qsqldatabase.h>
#include <qsqldriver.h>
#include <qsqlerror.h>

// QSql can suck it
#define protected public
#define private public
#include <qsqlresult.h>
#undef protected
#undef private

#include <qsqlquery.h>

#include "connection.h"
#include "database.h"
#include "expression.h"
#include "iniconfig.h"
#include "sqlerrorhandler.h"
#include "pgconnection.h"
#include "pyembed.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

int PyGILState_Check2(void);

namespace Stone {

SqlException::SqlException( const QString & sql, const QString & error )
: mSql(sql)
, mError(error)
{}

QString SqlException::sql() const
{ return mSql; }

QString SqlException::error() const
{ return mError; }

const char * SqlException::what() const throw()
{
	if( mCString.isNull() )
		mCString = QString("%1: %2").arg(mError).arg(mSql).toUtf8();
	return mCString.constData();
}

LostConnectionException::LostConnectionException()
{}

const char * LostConnectionException::what() const throw()
{ return "Lost Connection"; }

Connection::Connection(QObject * parent)
: QObject( parent )
, mPort( 0 )
, mReconnectDelay(10)
, mConnectionAttempts(0)
, mMaxConnectionAttempts(-1)
{}

Connection * Connection::create( const QString & dbType )
{
	if( dbType == "QPSQL7" ) {
		PGConnection * c = new PGConnection();
		return c;
	}
	LOG_1( "Unable to create connection for dbType: " + dbType );
	return 0;
}

Connection * Connection::createFromIni( IniConfig & cfg, const QString & section )
{
	cfg.pushSection( section );
	Connection * ret = create( cfg.readString( "DatabaseDriver" ) );
	if( ret )
		ret->setOptionsFromIni(cfg);
	cfg.popSection();
	Q_ASSERT(ret != 0);
	return ret;
}

void Connection::setHost( const QString & host )
{
	mHost = host;
}

void Connection::setPort( int port )
{
	LOG_3( "Port set to " + QString::number(port) );
	mPort = port;
}

void Connection::setDatabaseName( const QString & dbName )
{
	mDatabaseName = dbName;
}

void Connection::setUserName( const QString & userName )
{
	mUserName = userName;
}

void Connection::setPassword( const QString & password )
{
	mPassword = password;
}

void Connection::setReconnectDelay( int reconnectDelay )
{
	mReconnectDelay = reconnectDelay;
}

void Connection::setMaxConnectionAttempts( int maxConnectionAttempts )
{
	mMaxConnectionAttempts = maxConnectionAttempts;
}

void Connection::setOptionsFromIni( const IniConfig & cfg )
{
	setHost( cfg.readString( "Host" ) );
	setPort( cfg.readInt( "Port" ) );
	setDatabaseName( cfg.readString( "DatabaseName" ) );
	setUserName( cfg.readString( "User" ) );
	setPassword( cfg.readString( "Password" ) );
	setReconnectDelay( cfg.readInt( "ReconnectDelay", 30 ) );
	setMaxConnectionAttempts( cfg.readInt( "MaxConnectionAttempts", -1 ) );
	mPythonStackTraceOnTables = cfg.readString( "PythonStackTraceOnTables" ).split(",",QString::SkipEmptyParts);
}

bool Connection::checkConnection()
{
	// Retry once right away
	if( !isConnected() && !reconnect() )
	{
		emit connectionLost();
		emit connectionLost2();
		// Now either quit should have been called
		// or we are reconnected
	}
	return isConnected();
}

static int qSqlConnNumber()
{
	static int sConnNumber = 1;
	return sConnNumber++;
}

QSqlDbConnection::QSqlDbConnection( const QString & driverName )
: mInsideTransaction( false )
{
	mDb = QSqlDatabase::addDatabase( driverName, "QSqlDbConnection" + QString::number(qSqlConnNumber()) );
	mDatabaseType = driverName;
}

QSqlDbConnection::~QSqlDbConnection()
{
	if( mDb.isOpen() )
		mDb.close();
}

void QSqlDbConnection::setOptionsFromIni( const IniConfig & cfg )
{
	QStringList opts = cfg.readString( "Options" ).split( ";" );
	int timeout = cfg.readInt( "Timeout", 0 );
	if( timeout > 0 )
		opts += ("connect_timeout=" + QString::number(timeout));
	mExtraConnectOptions = opts.join( ";" );
	Connection::setOptionsFromIni(cfg);
}

void QSqlDbConnection::setupSqlDbOptions()
{
	mDb.setHostName( mHost );
	mDb.setPort( mPort );
	mDb.setDatabaseName( mDatabaseName );
	mDb.setUserName( mUserName );
	mDb.setPassword( mPassword );
	mDb.setConnectOptions( mExtraConnectOptions );
}

Connection::Capabilities QSqlDbConnection::capabilities() const
{
	return mDb.driver()->hasFeature( QSqlDriver::Transactions ) ? Cap_Transactions : static_cast<Connection::Capabilities>(0);
}

static int getQueryType( const QSqlQuery & query )
{
	QString q = query.lastQuery().simplified();
	if( q.startsWith( "select", Qt::CaseInsensitive ) )
		return Database::EchoSelect;
	else if( q.startsWith( "update", Qt::CaseInsensitive ) )
		return Database::EchoUpdate;
	else if( q.startsWith( "insert", Qt::CaseInsensitive ) )
		return Database::EchoInsert;
	else if( q.startsWith( "delete", Qt::CaseInsensitive ) )
		return Database::EchoDelete;
	return -1;
}

static QString queryNameFromType( int queryType )
{
	switch( queryType ) {
		case Database::EchoSelect:
			return QString("SELECT");
		case Database::EchoUpdate:
			return QString("UPDATE");
		case Database::EchoInsert:
			return QString("INSERT");
		case Database::EchoDelete:
			return QString("DELETE");
	}
	return QString("UNKNOWN");
}

QSqlQuery QSqlDbConnection::fakePrepare( const QString & sql )
{
// We can use the QSqlResult hack on windows because the symbols arent exported
#ifdef Q_OS_WIN
	checkConnection();
	QSqlQuery q(mDb);
	q.prepare(sql);
#else
	QSqlResult * result = mDb.driver()->createResult();
	QSqlQuery q( result );
	result->QSqlResult::prepare( sql );
#endif
	return q;
}

QSqlQuery QSqlDbConnection::exec( const QString & sql, const QList<QVariant> & vars, bool reExecLostConn, Table * table )
{
#ifdef Q_OS_WIN
	if (sql.toLower().startsWith("create") || sql.toLower().startsWith("alter"))
	{
		checkConnection();
		QSqlQuery squery(mDb);
		squery.exec(sql);
		return squery;
	}
#endif
	QSqlQuery query = fakePrepare(sql);
	foreach( QVariant v, vars )
		query.addBindValue( v );
	exec( query, reExecLostConn, table, true );
	return query;
}

static QString boundValueString( const QSqlQuery & q )
{
	QStringList valueStrings;
	QMapIterator<QString, QVariant> i(q.boundValues());
	while (i.hasNext()) {
		QString valueString;
		i.next();
		if( !i.key().isEmpty() && i.key() != ":f" )
			valueString = i.key() + ":";
		valueString += i.value().toString();
		valueStrings += valueString;
	}
	if( valueStrings.size() )
		return "(" + valueStrings.join(",") + ")";
	return QString();
}

bool QSqlDbConnection::exec( QSqlQuery & query, bool reExecLostConn, Table * table )
{
	return exec( query, reExecLostConn, table, false );
}

bool QSqlDbConnection::exec( QSqlQuery & query, bool reExecLostConn, Table * table, bool usingFakePrepareHack )
{
	bool result = false;
	int queryType = getQueryType( query );
	if( table && mPythonStackTraceOnTables.contains(table->tableName().toLower()) ) {
		printPythonStackTrace();
		LOG_1("ThrowOnTables contains " + table->tableName().toLower() +":\n" + query.lastQuery().simplified() );
               //throw SqlException( query.lastQuery().simplified(), "ThrowOnTables contains " + table->tableName().toLower() );
       }

       PyThreadState *_save = 0;
       if( Py_IsInitialized() && PyEval_ThreadsInitialized() && PyGILState_Check2())
               _save = PyEval_SaveThread();

       try {
               do {
                       // If gui dialog is connected to the connectionLost signal
                       // then it will block in checkConnection until a connection
                       // is made(or the user quits the app).
                       if( isConnected() || (((mMaxConnectionAttempts < 0) || (mConnectionAttempts < mMaxConnectionAttempts)) && checkConnection()) ) {
                               QTime time;
                               time.start();
       #ifndef Q_OS_WIN
                               if( usingFakePrepareHack ) {
                                       QSqlResult * qsr = (QSqlResult*)query.result();
                                       qsr->resetBindCount();

                                       if (qsr->lastError().isValid())
                                               qsr->setLastError(QSqlError());

                                       result = qsr->QSqlResult::exec();
                               } else
       #endif
                                       result = query.exec();

                               if( result ) {
                                       if( table )
                                               table->addSqlTime( time.elapsed(), queryType );

                                       Database * db = table ? table->database() : Database::current();
                                       if( queryType > 0 && db && (db->echoMode() & queryType) ) {
                                               LOG_3( query.executedQuery() + boundValueString(query) );
                                       }
                                       break;
                               } else {
                                       QSqlError error = query.lastError();
                                       if( !isConnected() || isConnectionError(error) ) {
                                               LOG_1( "Connection Error During " + queryNameFromType( queryType ) + (reExecLostConn ? (", retrying") : QString()) );
                                               if( mInsideTransaction ) {
                                                       throw LostConnectionException();
                                               }
                                               reconnect();
                                               continue;
                                       }
                                       mLastErrorText = query.executedQuery() + "\n" + error.text();
                                       LOG_1( mLastErrorText );
                                       SqlErrorHandler::instance()->handleError( mLastErrorText );
                                       throw SqlException(query.executedQuery(), mLastErrorText);
                                       break;
				}
			} else {
				if( mMaxConnectionAttempts >= 0 && mConnectionAttempts++ >= mMaxConnectionAttempts )
					break;
				QDateTime now = QDateTime::currentDateTime();
				do {
                                       // This will only happen in a non-gui application, otherwise the lost connection
                                       // dialog in stonegui will enter it's event look inside the check connection call above.
                                       QTime t;
                                       t.start();
                                       QCoreApplication::instance()->processEvents( QEventLoop::WaitForMoreEvents, 50 );
                                       if( t.elapsed() < 50 ) {
                                               //LOG_5( "QApplication::processEvents returned before the timeout period, we must be outside the event loop" );
#ifdef Q_OS_WIN
                                               Sleep( 1000 );
#else
                                               sleep( 1 );
#endif
					}
				} while( now.secsTo( QDateTime::currentDateTime() ) < mReconnectDelay );
			}
		} while( reExecLostConn );

		if( _save ) {
			PyEval_RestoreThread(_save);
			_save = 0;
		}
	} catch (...) {
		if( _save )
		{
			PyEval_RestoreThread(_save);
		}
		throw;
	}

	return result;
}

RecordList Connection::executeExpression( Table * table, FieldList fields, const Expression & exp )
{
	RecordList ret;
	
	QSqlQuery sq = exec( exp.toString(Expression::QueryString), QList<QVariant>(), true /*retry*/, table );
	while( sq.next() )
		ret += Record( new RecordImp( table, sq, 0, &fields ), false );
	return ret;
}

QMap<Table*,RecordList> Connection::executeExpression( Table * table, const RecordReturn & rr, const Expression & exp )
{
	Schema * schema = table->schema()->schema();
	QSqlQuery sq = exec( exp.toString(Expression::QueryString), QList<QVariant>(), true /*retry*/, table );
	QMap<Table*,RecordList> ret;
	while( sq.next() ) {
		Table * t = rr.table;
		if( rr.tableOidPos >= 0 ) {
			TableSchema * ts = tableByOid( sq.value(rr.tableOidPos).toUInt(), schema );
			if( ts )
				t = ts->table();
			else {
				LOG_1( "Unable to load record without table" );
				continue;
			}
		}
		if( t ) {
			QMap<Table*,QVector<int> >::const_iterator it = rr.columnPositions.find(t);
			if( it != rr.columnPositions.end() )
				ret[t] += Record( new RecordImp( t, sq, *it ), false );
		}
	}
	return ret;
}

TableSchema * Connection::tableByOid( uint oid, Schema * )
{
	return 0;
}

uint Connection::oidByTable( TableSchema * )
{
	return -1;
}


QStringList Connection::pythonStackTraceOnTables() const
{
	return mPythonStackTraceOnTables;
}

void Connection::setPythonStackTraceOnTables( const QStringList & tables )
{
	mPythonStackTraceOnTables = tables;
}

void Connection::addPythonStackTraceOnTable( const QString & tableName )
{
	if( !mPythonStackTraceOnTables.contains(tableName) )
		mPythonStackTraceOnTables.append(tableName);
}

void Connection::removePythonStackTraceOnTable( const QString & tableName )
{
	mPythonStackTraceOnTables.removeAll(tableName);
}


#if 0
QList<RecordList> Connection::executeExpression( Table * table, QList<RecordReturn> rrl, const Expression & exp )
{
	Schema * schema = table->schema()->schema();
	QList<RecordList> ret;
	while(ret.size()) < rrl.size()) ret.append(RecordList());
	
	QSqlQuery sq = exec( exp.toString(Expression::QueryString), QList<QVariant>(), true /*retry*/, table );
	while( sq.next() ) {
		int retCol = 0;
		foreach( const RecordReturn & rr, rrl ) {
			Table * t = rr.table;
			if( rr.tableOidPos >= 0 ) {
				TableSchema * ts = tableByOid( sq.value(rr.tableOidPos).toUInt(), schema );
				if( ts )
					t = ts->table();
				else {
					LOG_1( "Unable to load record without table" );
					continue;
				}
			}
			if( t ) {
				QMap<Table*,QVector<int> >::iterator it = rr.columnPositions.find(t);
				if( it != rr.columnPositions.end() )
					ret[retCol] += Record( new RecordImp( t, sq, *it ), false );
			}
			retCol++;
		}
	}
	return ret;
}
#endif

bool QSqlDbConnection::reconnect()
{
	if( mDb.isOpen() )
		mDb.close();
	mInsideTransaction = false;
	mDb.setHostName( mHost );
	mDb.setPort( mPort );
	mDb.setDatabaseName( mDatabaseName );
	mDb.setUserName( mUserName );
	mDb.setPassword( mPassword );
	mDb.setConnectOptions( mExtraConnectOptions );
	LOG_3( "Connecting... " + connectString() );
	if( !mDb.open() ){
		mLastErrorText = mDb.lastError().driverText();
		LOG_3( "Could not reconnect to database: " + mLastErrorText );
		return false;
	}
	mConnectionAttempts = 0;
	emit connected();
	return true;
}
	
bool QSqlDbConnection::isConnected()
{
	return mDb.isOpen();
}

bool QSqlDbConnection::closeConnection()
{
	mDb.close();
	return true;
}

QString QSqlDbConnection::connectString()
{
	return "Host: " + mDb.hostName() + " Port: " + QString::number(mDb.port()) + " Database: " + mDb.databaseName() + " User: " + mDb.userName() + " Options: " + mDb.connectOptions();
}

bool QSqlDbConnection::isConnectionError( const QSqlError & e )
{
	if( e.type() == QSqlError::ConnectionError
		|| e.databaseText().contains( "server closed the connection" )
		|| e.databaseText().contains( "Software caused connection abort" )
		|| e.databaseText().contains( "Connection timed out" )
		// Seems that connection errors sometimes give no database text, just Unable to create query
		|| (e.databaseText().isEmpty() && e.driverText().contains( "Unable to create query" )) )
		return true;
	return false;
}

bool QSqlDbConnection::beginTransaction()
{
	Database * db = Database::current();
	if( db && (db->echoMode() & (Database::EchoInsert | Database::EchoUpdate | Database::EchoDelete) ) )
		LOG_3( "BEGIN;" );
	
	return (mInsideTransaction = mDb.transaction());
}

bool QSqlDbConnection::commitTransaction()
{
	Database * db = Database::current();
	if( db && (db->echoMode() & (Database::EchoInsert | Database::EchoUpdate | Database::EchoDelete) ) )
		LOG_3( "COMMIT;" );
	mInsideTransaction = false;
	return mDb.commit();
}

bool QSqlDbConnection::rollbackTransaction()
{
	Database * db = Database::current();
	if( db && (db->echoMode() & (Database::EchoInsert | Database::EchoUpdate | Database::EchoDelete) ) )
		LOG_3( "ROLLBACK;" );
	mInsideTransaction = false;
	return mDb.rollback();
}

void QSqlDbConnection::listen( const QString & notificationName )
{
	mDb.driver()->subscribeToNotification(notificationName);
}

} //namespace

