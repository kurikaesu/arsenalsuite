
class Table : QObject
{
%TypeHeaderCode
#include <table.h>
%End
public:
	Table( Database * db /TransferThis/, TableSchema * schema );
	virtual ~Table();

	Database * database() const;
	
	TableSchema * schema() const;
	
	Connection * connection() const;

	QString tableName();

	Table * parent() const;
	QList<Table*> inherits();

	QList<Table*> children();

	QList<Table*> tableTree();

//	uint newPrimaryKey();

	QList<Index*> indexes();
	Index * indexFromSchema( IndexSchema * schema ) const;
	Index * indexFromField( const QString & name ) const;
	Index * index( const QString & name ) const;

	void addIndex( Index * index /Transfer/ );
	void removeIndex( Index * index, bool dontDelete = false );

	KeyIndex * keyIndex() const;

	void clearIndexes();

	void preload();
	bool isPreloaded() const;
	
	void preloadProject( uint fkeyProject );
	bool isProjectPreloaded( uint fkeyElement, bool preloadNow = false );
	
	MappedRecord load( QVariant * v = 0 );
	MappedRecord * newObject();

	//MappedRecord record( uint key, bool select=true, bool useCache=true, bool baseOnly=false );
	MappedRecord record( uint key, int lookupMode = Index::UseCache | Index::UseSelect, bool baseOnly=false ) throw(SqlException,LostConnectionException,PythonException);

	MappedRecordList records( QList<uint> keys ) throw(SqlException,LostConnectionException,PythonException);
	MappedRecordList records( const QString & keystring ) throw(SqlException,LostConnectionException,PythonException);
	
	JoinedSelect join( Table * table, QString condition = QString(), JoinType joinType = InnerJoin, bool ignoreResults = false, const QString & alias = QString() );

	RecordList selectFrom( const QString & from, const QList<QVariant> & args = VarList() ) throw(SqlException,LostConnectionException,PythonException);

	MappedRecordList select( 
		const QString & where = QString::null,
		const QList<QVariant> & args = QList<QVariant>(),
		bool selectChildren = true,
		bool expectSingle = false,
		bool needResults = true ) throw(SqlException,LostConnectionException,PythonException);

	MappedRecordList select( 
		const Expression & exp,
		bool selectChildren = true,
		bool expectSingle = false,
		bool needResults = true ) throw(SqlException,LostConnectionException,PythonException);

	MappedRecordList selectOnly( const QString & where, const QList<QVariant> & args = QList<QVariant>(), bool needResults = true ) throw(SqlException,LostConnectionException,PythonException);
	MappedRecordList selectOnly( const Expression & exp, bool needResults = true, bool cacheIncoming = false ) throw(SqlException,LostConnectionException,PythonException);

	bool insert( const Record & rb ) throw(SqlException,LostConnectionException,PythonException);
	bool insert( const RecordList & rb ) throw(SqlException,LostConnectionException,PythonException);

	void update( RecordImp * r ) throw(SqlException,LostConnectionException,PythonException);
	int remove( const Record & rb ) throw(SqlException,LostConnectionException,PythonException);
	int remove( const RecordList & ) throw(SqlException,LostConnectionException,PythonException);

	bool exists() const throw(SqlException,LostConnectionException,PythonException);
	bool verifyTable( bool createMissingColumns = false, QString * output=0 ) throw(SqlException,LostConnectionException,PythonException);
	bool createTable( QString * output = 0 ) throw(SqlException,LostConnectionException,PythonException);

	void setCacheIncoming( bool ci );

	enum {
		SqlInsert,
		SqlSelect,
		SqlUpdate,
		SqlDelete,
		SqlAll
	};

	enum {
		IndexAdded,
		IndexRemoved,
		IndexUpdated,
		IndexIncoming,
		IndexSearch,
		IndexAll
	};

	void addSqlTime( int ms, int action );
	int elapsedSqlTime( int action = SqlAll ) const;

	void addIndexTime( int ms, int action );
	int elapsedIndexTime( int action = IndexAll ) const;

	void printStats();
	
	// returns BORROWED ref
	RecordImp * emptyImp();

	int mImpCount;
signals:
	void added( RecordList );
	void removed( RecordList );
	void updated( Record, Record );

protected slots:
	virtual void recordsAdded( RecordList, bool notifyIndexes = true );
	virtual void recordsRemoved( RecordList, bool notifyIndexes = true );
	virtual void recordUpdated( const Record &, const Record &, bool notifyIndexes = true );
};

