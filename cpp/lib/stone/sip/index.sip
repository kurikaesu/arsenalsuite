

class Index /Abstract/
{
%TypeHeaderCode
#include <index.h>
%End
public:

%ConvertToSubClassCode
	sipClass = NULL;
	if( dynamic_cast<HashIndex*>(sipCpp) )
		sipClass = sipClass_HashIndex;
	else if( dynamic_cast<KeyIndex*>(sipCpp) )
		sipClass = sipClass_KeyIndex;
%End

	Index( Table * table /TransferThis/, IndexSchema * schema, bool useCache = false );
	virtual ~Index();

	enum {
		NoInfo,
		RecordsFound
	};

	enum LookupMode {
		UseCache = 0x1,
		UseSelect = 0x2,
		PartialSelect = 0x4
	};

	IndexSchema * schema() const;
	Table * table() const;

	void setCacheEnabled( bool cacheEnabled );
	bool cacheEnabled() const;

	void cacheIncoming( bool ci );

	MappedRecordList recordsByIndexMulti( const QList<QVariant> & args, int lookupMode = UseCache | UseSelect | PartialSelect ) throw(SqlException,LostConnectionException);
	//MappedRecordList recordsByIndexMulti( const QList<QVariant> & args, bool select );

	//MappedRecordList recordsByIndex( const QList<QVariant> & args, bool select );
	//MappedRecord recordByIndex( const QVariant & arg, bool select );

	MappedRecordList recordsByIndex( const QList<QVariant> & args, int lookupMode = UseCache | UseSelect ) throw(SqlException,LostConnectionException);
	MappedRecord recordByIndex( const QVariant & arg, int lookupMode = UseCache | UseSelect ) throw(SqlException,LostConnectionException);

	virtual MappedRecordList records( QList<QVariant> vars, int & status ) =0;
	virtual void setEmptyEntry( QList<QVariant> vars )=0;

	virtual void recordAdded( const Record & )=0;
	virtual void recordRemoved( const Record & )=0;
	virtual void recordUpdated( const Record &, const Record & )=0;
	virtual void recordsIncoming( const RecordList &, bool ci = false )=0;
	virtual void clear()=0;
	void printStats();
private:
	Index(const Index &);
};

class HashIndex : Index
{
%TypeHeaderCode
#include <index.h>
%End
public:
	HashIndex( Table * parent /TransferThis/, IndexSchema * );
	~HashIndex();

	virtual MappedRecordList records( QList<QVariant> vars, int & status );
	virtual void setEmptyEntry( QList<QVariant> vars );

	virtual void recordAdded( const Record & );
	virtual void recordRemoved( const Record & );
	virtual void recordUpdated( const Record &, const Record & );
	virtual void recordsIncoming( const RecordList &, bool ci = false );
	virtual void clear();
private:
	HashIndex( const HashIndex & );
};

class KeyIndex : Index
{
%TypeHeaderCode
#include <index.h>
%End
public:
	KeyIndex( Table * parent /TransferThis/, IndexSchema * );
	~KeyIndex();

	MappedRecord record( unsigned int key, bool * foundEntry );

	MappedRecordList records( QList<QVariant> vars, int & status );
	void setEmptyEntry( QList<QVariant> vars );
	
	MappedRecordList values();
private:
	KeyIndex( const KeyIndex & );
};
