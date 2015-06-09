
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

#ifndef DATABASE_H
#define DATABASE_H

#include <qobject.h>
#include <qmap.h>
#include <qsqlquery.h>
#include <qstack.h>
#include <qthreadstorage.h>

#include "table.h"
#include "updatemanager.h"
#include "record.h"
#include "connection.h"

class QTimer;
class QSqlQuery;
class RecordList;

namespace Stone {
class Schema;
class Table;
class UpdateManager;

/** The Database class encapsulates a group of tables.
 *  It provides events when data is changed in those tables.
 *  It keeps track of the sql execution time.  It controls
 *  whether or not to log sql.  It controls transactions.
 *  \ingroup Stone
 */
class STONE_EXPORT Database : public QObject
{
Q_OBJECT
public:
	Database( Schema * schema, Connection * conn = 0 );
	~Database();

	void ref();
	void deref();

	/** Returns the current database for this thread and schema.
	 *  If schema is 0, returns the last database set with setCurrent
	 *  for this thread. */
	static Database * current( Schema * schema = 0 );

	/** Returns all database set for this thread */
	static QList<Database*> databases();

	/** Sets the current database for this thread
     *  it is safe to call this function multiple times
	 *  for the same database, and also to call this function
	 *  with the same database for multiple threads.
	 *  Each database has a reference count and is deleted
	 *  when the last thread accessing it is terminated */
	static void setCurrent( Database * );

	Connection * connection() const;
	/// Deletes the current connection, takes ownership
	/// of the new Connection.
	void setConnection( Connection * c );

	/// These methods are simply forwarded to connection
	/// and documented there, only used to allow shorter code.
	QSqlQuery exec( const QString & sql, const QList<QVariant> & vars = QList<QVariant>(), bool reExecLostConn = true, Table * table = 0 )
	{ return connection()->exec( sql, vars, reExecLostConn, table ); }

	bool exec( QSqlQuery & query, bool reExecLostConn = true, Table * table = 0 )
	{ return connection()->exec( query, reExecLostConn, table ); }


	Schema * schema() const { return mSchema; }

	/** Case insensitive
	 */
	Table * tableByName( const QString & tableName ) const;
	/** Case insensitive
	 */
	Table * tableByClass( const QString & className ) const;
	
	Table * tableFromSchema( TableSchema * table ) const;

	/** Returns all the tables associated with this database
	 */
	TableList tables();

	enum {
		EchoSelect=1,
		EchoUpdate=2,
		EchoInsert=4,
		EchoDelete=8
	};

	/** Sets the echo mode.  This controls which sql statements
	 *  are logged to the multilog(file and/or stdout).
	 */
	void setEchoMode( uint echoMode );

	/** Returns the current echo mode
	 */
	uint echoMode() const;

	/** Returns true if the UndoManager is enabled for this database.
	 */
	bool undoEnabled() const;

	/** Enables/Disables the UndoManager according to \param enabled
	 */
	void setUndoEnabled( bool enabled );

	/** Returns the number of milliseconds spent executing sql.
	 *  \param action determines which types of sql statements to
	 * add to the returned time total.
	 */
	int elapsedSqlTime( int action = Table::SqlAll );

	/** Returns the number of milliseconds spent in the indexes.
	 *  \param action determines what type of functions to
	 * add to the returned time total.
	 */
	int elapsedIndexTime( int action = Table::IndexAll );

	/** Starts a database transaction, with the \param title.
	 *  Transactions can be nested.
	 */
	void beginTransaction( const QString & title = QString::null );

	/** Commits the current transaction.
	 */
	void commitTransaction();

	/** Rolls back the current transaction.  All nested transaction are invalid
	 *  and nothing will be committed until they are unwound with further
	 *  calls to rollback transaction, or commit transaction */
	void rollbackTransaction();
	
	/** Used before executing sql to ensure that we have begun a transaction at the
	 *  actual database level. 
	 */
	bool ensureInsideTransaction();

	/** Sends all queued updates to the update manager. Called when
	 *  a transaction is committed.
	 */
	void flushUpdateBuffer();

	/** Returns true if the transaction stack is not empty. */
	bool insideTransaction();

	/** Verifies that the internal schema matches the schema in the database.
	 *  Returns true if there match, else returns false and fills output
	 *  with text describing missing/conflicting schema items. */
	bool verifyTables( QString * output = 0 );

	/** Creates any tables in the database that are missing according to
	 *  the internal schema.
	 */
	bool createTables( QString * output = 0 );

	/** This is used internally by the indexes for (Cascade|Update)OnDelete
	*/ 
	void addDeleteAction( Field * );
	void removeDeleteAction( Field * );

	/** Prints all the index and sql stats to the log. */
	void printStats();

	/// Returns the current setting for queueing record signals
	/// If this value is true, the recordsAdded, recordsRemoved, and recordUpdated
	/// signals are delivered via Qt::QueuedConnection(after control is returned to the qt 
	/// mainloop), so that there is no worry's about functions being reentrant.
	/// If this value is false, the signals are delivered right away.
	bool queueRecordSignals() const;

	/// Sets the current value directly, ignoring any queued values.
	bool setQueueRecordSignals( bool qrc );

	/// Sets the current value of queueRecordSignals, and pushes the previous valud
	/// onto an internal stack, to be restored with popQueueRecordSignals.
	bool pushQueueRecordSignals( bool );

	/// Restores the queueRecordsSignals setting as it was before the last call
	/// to pushQueueRecordSignals.
	bool popQueueRecordSignals();

signals:

	/** Emitted anytime any records are added to any of the tables in this database */
	void recordsAddedSignal( RecordList );
	/** Emitted anytime any records are removed from any of the tables in this database */
	void recordsRemovedSignal( RecordList );
	/** Emitted anytime a record from this database is updated */
	void recordUpdatedSignal( Record current, Record updated );
	void recordsIncomingSignal( const RecordList & );

protected slots:
	void transactionTimeout();

	void dispatchNotification( const QString & name );

	void connectionConnected();
	void connectionLost();
protected:
	/** Called by the tables, to indicate when records have been added.
	 */
	void recordsAdded( const RecordList &, bool local = false );

	/** Called by the tables, to indicate when records have been removed.
	 */
	void recordsRemoved( const RecordList &, bool local = false );

	/** Called by the tables, to indicate when a record has been modified.
	 */
	void recordUpdated( const Record & current, const Record & updated, bool local = false );

	/** Called by the tables, to indicate records that have been selected.
	 *  Used by the indexes for caching.
	 */
	void recordsIncoming( const RecordList &, bool co = false );

	void setupConnectionNotifications( Connection * );

	bool setupPreloadListen( Table * table );
	
	// These delete actions need to be added to
	// not-yet-created tables.
	FieldList mPendingDeleteActions;
	
	Schema * mSchema;

	QHash<TableSchema *,Table*> mSchemaToTable;
	QThreadStorage<Connection *> mConnections;

	uint mEchoMode;
	
	// Incremented every time beginTransaction is called
	// decremented every time commitTransaction is called
	int mInsideTransactionCount;
	
	// Keeps track of whether or not we have really opened a transaction
	// We don't do it until we have to, because if nothing is updated or deleted,
	// then we can avoid two pointless roundtrips(begin; and end;)
	bool mReallyInsideTransaction;
	bool mRolledBack;

	bool mUndoEnabled;

	QTimer * mTransactionTimer;

	bool mQueueRecordSignals;
	QStack<bool> mQueueRecordSignalsStack;

	int mRefCount;

	friend class ::Table;
	friend class ::UpdateManager;
};

}
using namespace Stone;
using Stone::Database;

#endif // DATABASE_H

