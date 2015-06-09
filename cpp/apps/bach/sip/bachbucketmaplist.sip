


%MappedType MappedBachBucketMapList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachbucketmaplist.h"
#include "bachbucketmaptable.h"
#define MappedBachBucketMapList BachBucketMapList
%End

%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp, true, BachBucketMapSchema::instance() );
%End
};

class BachBucketMapList : RecordList
{
%TypeHeaderCode
#include <bachbucketmaplist.h>
#include "bachbucket.h"
#include "bachasset.h"

%End
public:
	BachBucketMapList();
	BachBucketMapList( const RecordList &, Table * t = 0 );
	BachBucketMapList( const BachBucketMapList & );
	BachBucketMapList( const Record &, Table * t = 0 );
	
	~BachBucketMapList();

//	void insert( BachBucketMapIter, const BachBucketMap & );

//	BachBucketMapIter at( uint );

//	BachBucketMapIter find( const Record & );

//	BachBucketMapIter remove( const BachBucketMapIter & );

	int remove( const Record & );
	
	int remove();
	
	//BachBucketMapIter begin();

	//BachBucketMapIter end();

	Table * table() const;
	TableSchema * schema() const;

	MappedBachBucketMapList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;
	MappedBachBucketMapList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;
	MappedBachBucketMapList sorted( const QString & c, bool a = true ) const;

	MappedBachBucketMapList unique() const;
	MappedBachBucketMapList reversed() const;
	MappedBachBucketMapList reloaded() const;

	MappedBachBucketList bachBuckets() const;
	MappedBachBucketMapList & setBachBuckets( const BachBucket & );
	MappedBachAssetList bachAssets() const;
	MappedBachBucketMapList & setBachAssets( const BachAsset & );
	QList<int> positions() const;
	MappedBachBucketMapList & setPositions( const int & );


};


