
class RecordImp
{
%TypeHeaderCode
#include <recordimp.h>
%End

public:
	RecordImp( Table * table, QVariant * toLoad=0 );
	RecordImp( Table * table, QSqlQuery & q, int queryPosOffset = 0 );

	~RecordImp();
	
	void ref();
	void deref();

	int refCount() const;

	QVariant getColumn( int col ) const throw(SqlException,LostConnectionException);
	RecordImp * setColumn( int col, const QVariant & v );

	QVariant getValue( const QString & column ) const throw(SqlException,LostConnectionException);
	RecordImp * setValue( const QString & column, const QVariant & v );
	
	Table * table() const;

	RecordImp * copy( bool attachToPristine = true );

	uint key() const;
	
	RecordImp * commit() throw(SqlException,LostConnectionException);

	void remove() throw(SqlException,LostConnectionException);

	void setColumnModified( uint col, bool modified );
	bool isColumnModified( uint col ) const;
	void clearModifiedBits();
	
	RecordImp * setColumnLiteral( uint col, bool modified );
	bool isColumnLiteral( uint col ) const;
	void clearColumnLiterals();
	
	enum State {
		COMMITTED = 1,
		MODIFIED = 2,
		DELETED = 4,
		EMPTY_SHARED = 8,
		COMMIT_ALL_FIELDS = 16,
		DISCARDED = 32,
		INSERT_PENDING = 64,
		UPDATE_PENDING = 128,
		DELETE_PENDING = 256,
		INSERT_PENDING_CHILD = 512
	};
	
	bool hasVersions() const;

	static int totalCount();
};

