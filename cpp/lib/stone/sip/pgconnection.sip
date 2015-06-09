
class PGConnection : QSqlDbConnection
{
%TypeHeaderCode
#include <pgconnection.h>
%End
public:
	PGConnection();
	
	virtual Capabilities capabilities() const;

	virtual bool tableExists( TableSchema * schema ) throw(SqlException,LostConnectionException);
	virtual bool verifyTable( TableSchema * schema, bool createMissingColumns = false, QString * output=0 ) throw(SqlException,LostConnectionException);
	virtual bool createTable( TableSchema * schema, QString * output = 0 ) throw(SqlException,LostConnectionException);

	virtual TableSchema * importTableSchema() throw(SqlException,LostConnectionException) /TransferBack/;

	virtual Schema * importDatabaseSchema() throw(SqlException,LostConnectionException) /TransferBack/;

	virtual uint newPrimaryKey( TableSchema * ) throw(SqlException,LostConnectionException);

	virtual RecordList selectFrom( Table * table, const QString & from, const QList<QVariant> & args ) throw(SqlException,LostConnectionException);

	virtual MappedRecordList selectOnly( Table *, const QString & where = QString::null, const QList<QVariant> & vars = QList<QVariant>() ) throw(SqlException,LostConnectionException);
	virtual QList<RecordList> joinedSelect( const JoinedSelect &, QString where, QList<QVariant> vars ) throw(SqlException,LostConnectionException);

	virtual void insert( Table *, const RecordList & rl ) throw(SqlException,LostConnectionException);

	virtual bool update( Table *, RecordImp * imp, Record * returnVals = 0 ) throw(SqlException,LostConnectionException);
	virtual bool update( Table * table, RecordList records, RecordList * returnValues = 0 ) throw(SqlException,LostConnectionException);

	virtual int remove( Table *, const QString &, QList<int> * rowsDeleted = 0 ) throw(SqlException,LostConnectionException);

	virtual bool createIndex( IndexSchema * schema ) throw(SqlException,LostConnectionException);
	
	bool checkVersion( int vMajor, int vMinor ) throw(SqlException,LostConnectionException);
};
