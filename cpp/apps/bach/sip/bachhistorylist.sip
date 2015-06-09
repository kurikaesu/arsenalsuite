


%MappedType MappedBachHistoryList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachhistorylist.h"
#include "bachhistorytable.h"
#define MappedBachHistoryList BachHistoryList
%End

%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp, true, BachHistorySchema::instance() );
%End
};

class BachHistoryList : RecordList
{
%TypeHeaderCode
#include <bachhistorylist.h>

%End
public:
	BachHistoryList();
	BachHistoryList( const RecordList &, Table * t = 0 );
	BachHistoryList( const BachHistoryList & );
	BachHistoryList( const Record &, Table * t = 0 );
	
	~BachHistoryList();

//	void insert( BachHistoryIter, const BachHistory & );

//	BachHistoryIter at( uint );

//	BachHistoryIter find( const Record & );

//	BachHistoryIter remove( const BachHistoryIter & );

	int remove( const Record & );
	
	int remove();
	
	//BachHistoryIter begin();

	//BachHistoryIter end();

	Table * table() const;
	TableSchema * schema() const;

	MappedBachHistoryList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;
	MappedBachHistoryList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;
	MappedBachHistoryList sorted( const QString & c, bool a = true ) const;

	MappedBachHistoryList unique() const;
	MappedBachHistoryList reversed() const;
	MappedBachHistoryList reloaded() const;

	QStringList actions() const;
	MappedBachHistoryList & setActions( const QString & );
	QStringList users() const;
	MappedBachHistoryList & setUsers( const QString & );
	QStringList tableNames() const;
	MappedBachHistoryList & setTableNames( const QString & );


};


