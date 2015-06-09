


%MappedType MappedBachBucketList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachbucketlist.h"
#include "bachbuckettable.h"
#define MappedBachBucketList BachBucketList
%End

%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp, true, BachBucketSchema::instance() );
%End
};

class BachBucketList : RecordList
{
%TypeHeaderCode
#include <bachbucketlist.h>
#include "bachnamespace.h"
#include "bachbucketmap.h"

%End
public:
	BachBucketList();
	BachBucketList( const RecordList &, Table * t = 0 );
	BachBucketList( const BachBucketList & );
	BachBucketList( const Record &, Table * t = 0 );
	
	~BachBucketList();

//	void insert( BachBucketIter, const BachBucket & );

//	BachBucketIter at( uint );

//	BachBucketIter find( const Record & );

//	BachBucketIter remove( const BachBucketIter & );

	int remove( const Record & );
	
	int remove();
	
	//BachBucketIter begin();

	//BachBucketIter end();

	Table * table() const;
	TableSchema * schema() const;

	MappedBachBucketList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;
	MappedBachBucketList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;
	MappedBachBucketList sorted( const QString & c, bool a = true ) const;

	MappedBachBucketList unique() const;
	MappedBachBucketList reversed() const;
	MappedBachBucketList reloaded() const;

	QStringList names() const;
	MappedBachBucketList & setNames( const QString & );
	MappedBachNamespaceList bachNamespaces() const;
	MappedBachBucketList & setBachNamespaces( const BachNamespace & );
	MappedBachBucketMapList bachBucketMaps();


};


