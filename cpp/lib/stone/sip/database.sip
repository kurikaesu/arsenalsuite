class Database : QObject
{
%TypeHeaderCode
#include <database.h>
%End

public:
	Database( Schema * schema, Connection * conn /Transfer/ = 0 );
	~Database();

	void ref();
	void deref();

	static Database * current( Schema * s = 0 );
	static void setCurrent( Database * /Transfer/ );
	static QList<Database*> databases();

	Connection * connection() const;
	void setConnection( Connection * c /Transfer/ );

	/// These methods are simply forwarded to connection
	/// and documented there, only used to allow shorter code.
	QSqlQuery exec( const QString & sql, const QList<QVariant> & vars = QList<QVariant>(), bool reExecLostConn = true, Table * table = 0 ) throw(SqlException,LostConnectionException) /PyName=exec_/;
	bool exec( QSqlQuery & query, bool reExecLostConn = true, Table * table = 0 ) throw(SqlException,LostConnectionException) /PyName=exec_/;

	bool exec( QSqlQuery & query, bool reExecLostConn = true, Table * table = 0 ) throw(SqlException,LostConnectionException);

	Schema * schema() const;

	Table * tableByName( const QString & tableName ) const;
	Table * tableByClass( const QString & className ) const;
	
	Table * tableFromSchema( TableSchema * table ) const;

	QList<Table*> tables();

	enum {
		EchoSelect=1,
		EchoUpdate=2,
		EchoInsert=4,
		EchoDelete=8
	};

	void setEchoMode( uint echoMode );

	uint echoMode() const;

	bool undoEnabled() const;

	void setUndoEnabled( bool enabled );

	int elapsedSqlTime( int action = Table::SqlAll );

	int elapsedIndexTime( int action = Table::IndexAll );

	void beginTransaction( const QString & title = QString::null );

	void commitTransaction() throw(SqlException,LostConnectionException);

	void rollbackTransaction() throw(SqlException,LostConnectionException);
	
	bool ensureInsideTransaction() throw(SqlException,LostConnectionException);

	void flushUpdateBuffer();

	bool insideTransaction();

	bool verifyTables( QString * output = 0 ) throw(SqlException,LostConnectionException);

	bool createTables( QString * output = 0 ) throw(SqlException,LostConnectionException);

	void addDeleteAction( Field * );
	void removeDeleteAction( Field * );

	void printStats();

	bool queueRecordSignals() const;

	bool setQueueRecordSignals( bool qrc );

	bool pushQueueRecordSignals( bool );

	bool popQueueRecordSignals();

signals:

	void recordsAddedSignal( RecordList );
	void recordsRemovedSignal( RecordList );
	void recordUpdatedSignal( Record current, Record updated );
	void recordsIncomingSignal( const RecordList & );

protected slots:
	void transactionTimeout();
};



